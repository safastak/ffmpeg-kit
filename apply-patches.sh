#!/bin/bash
#
# Applies patches from patches/<library>/ to src/<library>/
# Run automatically during build (hooked into android.sh/ios.sh)
# or manually: bash ./apply-patches.sh
#

BASEDIR="$(cd "$(dirname "$0")" && pwd)"
PATCHES_DIR="${BASEDIR}/patches"
SRC_DIR="${BASEDIR}/src"

echo "apply-patches: starting"

if [[ ! -d "${PATCHES_DIR}" ]]; then
  echo "apply-patches: no patches directory found, skipping."
  exit 0
fi

APPLIED=0
SKIPPED=0
FAILED=0

for lib_dir in "${PATCHES_DIR}"/*/; do
  [[ ! -d "${lib_dir}" ]] && continue

  lib_name=$(basename "${lib_dir}")
  target_dir="${SRC_DIR}/${lib_name}"

  if [[ ! -d "${target_dir}" ]]; then
    echo "apply-patches: WARN: src/${lib_name}/ not found — skipping"
    continue
  fi

  for patch_file in "${lib_dir}"*.patch; do
    [[ ! -f "${patch_file}" ]] && continue

    patch_name=$(basename "${patch_file}")

    # Try forward apply first
    # Use --batch --force to prevent any interactive prompts
    if patch -p1 --batch --forward --dry-run -d "${target_dir}" --input="${patch_file}" </dev/null >/dev/null 2>&1; then
      patch -p1 --batch --forward -d "${target_dir}" --input="${patch_file}" </dev/null >/dev/null 2>&1
      if [[ $? -eq 0 ]]; then
        echo "Applied: patches/${lib_name}/${patch_name}"
        APPLIED=$((APPLIED + 1))
      else
        echo "FAILED:  patches/${lib_name}/${patch_name}"
        FAILED=$((FAILED + 1))
      fi
    # If forward fails, check if already applied
    elif patch -p1 --batch -R --dry-run -d "${target_dir}" --input="${patch_file}" </dev/null >/dev/null 2>&1; then
      echo "Already applied: patches/${lib_name}/${patch_name}"
      SKIPPED=$((SKIPPED + 1))
    else
      echo "FAILED (won't apply cleanly): patches/${lib_name}/${patch_name}"
      FAILED=$((FAILED + 1))
    fi
  done
done

echo ""
echo "Patches: ${APPLIED} applied, ${SKIPPED} already applied, ${FAILED} failed"

if [[ ${FAILED} -gt 0 ]]; then
  exit 1
fi
