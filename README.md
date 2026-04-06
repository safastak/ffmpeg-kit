# FFmpeg-Kit — Safastak Build Guide

This is our fork of the archived [ffmpeg-kit](https://github.com/arthenica/ffmpeg-kit) project, configured to build FFmpeg 7.1 with LGPL external libraries for both Android and iOS.

See `README-original.md` for the upstream project documentation.

## Prerequisites

### Common
- macOS (required for iOS builds)
- Git
- At least 20GB free disk space (source downloads + build artifacts)

### Android
- **Android SDK** — typically at `~/Library/Android/sdk` (installed via Android Studio)
- **Android SDK Command-Line Tools + NDK r28** — run as one block:
  ```bash
  export ANDROID_HOME=~/Library/Android/sdk
  export ANDROID_SDK_ROOT=~/Library/Android/sdk
  export JAVA_HOME="/Applications/Android Studio.app/Contents/jbr/Contents/Home"

  # Install cmdline-tools (skip if already installed)
  cd /tmp
  curl -o cmdline-tools.zip https://dl.google.com/android/repository/commandlinetools-mac-11076708_latest.zip
  unzip -o cmdline-tools.zip
  mkdir -p "$ANDROID_HOME/cmdline-tools/latest"
  mv cmdline-tools/* "$ANDROID_HOME/cmdline-tools/latest/"
  rm -rf cmdline-tools cmdline-tools.zip

  # Install NDK r28 (required for 16KB page alignment)
  "$ANDROID_HOME/cmdline-tools/latest/bin/sdkmanager" "ndk;28.0.13004108"

  # Set NDK path
  export ANDROID_NDK_ROOT="$ANDROID_HOME/ndk/28.0.13004108"
  ```

### iOS
- **Xcode** (latest stable) with command line tools
- `xcrun` and `xcodebuild` must be available

## Build Commands

### Android (LTS build)

```bash
export ANDROID_HOME=~/Library/Android/sdk
export ANDROID_SDK_ROOT=~/Library/Android/sdk
export ANDROID_NDK_ROOT="$ANDROID_HOME/ndk/28.0.13004108"
export JAVA_HOME="/Applications/Android Studio.app/Contents/jbr/Contents/Home"

./android.sh --lts \
  --enable-dav1d \
  --enable-fontconfig \
  --enable-freetype \
  --enable-fribidi \
  --enable-kvazaar \
  --enable-lame \
  --enable-libass \
  --enable-libiconv \
  --enable-libilbc \
  --enable-libtheora \
  --enable-libvorbis \
  --enable-libvpx \
  --enable-libwebp \
  --enable-libxml2 \
  --enable-opencore-amr \
  --enable-opus \
  --enable-shine \
  --enable-snappy \
  --enable-soxr \
  --enable-speex \
  --enable-twolame \
  --enable-vo-amrwbenc \
  --enable-zimg \
  --enable-android-media-codec \
  --enable-android-zlib
```

Build time: ~30–60 minutes.

**Output:** `prebuilt/bundle-android-aar-lts/ffmpeg-kit/ffmpeg-kit.aar`

### iOS (XCFramework build)

```bash
./ios.sh -x \
  --disable-x86-64-mac-catalyst \
  --enable-dav1d \
  --enable-fontconfig \
  --enable-freetype \
  --enable-fribidi \
  --enable-kvazaar \
  --enable-lame \
  --enable-libass \
  --enable-ios-libiconv \
  --enable-libilbc \
  --enable-libtheora \
  --enable-libvorbis \
  --enable-libvpx \
  --enable-libwebp \
  --enable-libxml2 \
  --enable-opencore-amr \
  --enable-opus \
  --enable-shine \
  --enable-snappy \
  --enable-soxr \
  --enable-speex \
  --enable-twolame \
  --enable-vo-amrwbenc \
  --enable-zimg
```

Note: zlib is enabled automatically via a patch in `scripts/apple/ffmpeg.sh`.
Note: `--disable-x86-64` skips the Intel simulator arch — unnecessary on Apple Silicon Macs and fails with newer Xcode SDKs.

Build time: ~30–60 minutes.

**Output:** `prebuilt/bundle-apple-xcframework-ios/` (contains 8 `.xcframework` bundles)

## Post-Build: Copy Artifacts to StickersApp

### Android

```bash
STICKERSAPP=~/src/safastak/stickersapp/stickersapp

# Copy the new AAR
cp prebuilt/bundle-android-aar-lts/ffmpeg-kit/ffmpeg-kit.aar \
  "$STICKERSAPP/apps/expo/modules/shared/scripts/android/lib/ffmpeg-kit-full-7.1.aar"

# Remove the old AAR
rm "$STICKERSAPP/apps/expo/modules/shared/scripts/android/lib/ffmpeg-kit-full-6.0-2.aar"
```

### iOS

The stickersapp iOS build references `~/src/ffmpeg-kit-ios-full-lgpl` as a local CocoaPod (via the `withFfmpegKit.js` Expo config plugin). After building, replace the frameworks there:

```bash
IOS_POD=~/src/ffmpeg-kit-ios-full-lgpl

# Remove old frameworks
rm -rf "$IOS_POD/Frameworks/"*.xcframework

# Copy new frameworks
cp -R prebuilt/bundle-apple-xcframework-ios/*.xcframework "$IOS_POD/Frameworks/"
```

Then update the podspec version in `$IOS_POD/ffmpeg-kit-ios-full-lgpl.podspec`:
```ruby
s.version = '7.1'
```

## Verifying 16KB Page Alignment (Android)

After building, extract and verify the `.so` files from the AAR:

```bash
cd /tmp
rm -rf aar-check && mkdir aar-check && cd aar-check
unzip -o ~/src/ffmpeg-kit/prebuilt/bundle-android-aar-lts/ffmpeg-kit/ffmpeg-kit.aar "jni/**/*.so"

# Check alignment — LOAD segments should show 0x4000
for so in $(find jni -name "*.so"); do
  echo "=== $so ==="
  readelf -l "$so" 2>/dev/null | awk '/LOAD/{getline; print $NF}'
done
```

Expected: all LOAD segments show `0x4000` (16KB). If you see `0x1000` (4KB), the alignment flags didn't take effect.

## Build Configuration

### FFmpeg Version
Configured in `scripts/source.sh` under the `ffmpeg)` case — currently set to tag `n7.1`.

### External Library Versions
All external library versions are defined in `scripts/source.sh`. Each library has a repo URL and tag/commit reference.

### NDK Version
Set in `tools/android/build.gradle` and `tools/android/build.lts.gradle` — currently `28.0.13004108`.

### 16KB Page Alignment
Linker flags `-Wl,-z,max-page-size=16384 -Wl,-z,common-page-size=16384` are applied in two places in `scripts/function-android.sh`:
- `get_ldflags()` — affects all external library `.so` builds
- `build_application_mk()` APP_LDFLAGS — affects the ffmpegkit native library build

### Patches
Source patches live in `patches/<library>/`. They are applied automatically after source download during `android.sh` / `ios.sh`, or manually via:
```bash
./apply-patches.sh
```

To add a new patch:
1. Create `patches/<library>/description.patch` (unified diff format, `-p1` relative to `src/<library>/`)
2. It will be applied automatically on next build

Current patches:
- `patches/libuuid/fix-flock-include.patch` — adds missing `<sys/file.h>` include for NDK r28 compatibility

### License
LGPL build — no GPL libraries are enabled. Do not add `--enable-gpl` or enable x264/x265/xvidcore.

## Troubleshooting

- **Build logs:** Check `build.log` in the repo root
- **Clean rebuild:** Delete `src/` (downloaded sources) and `prebuilt/` (build outputs), then re-run
- **Source download failures:** Some libs use `safastak/*` forks (lame, libpng, soxr, srt, chromaprint) — ensure you have access to those repos
- **iOS arm64-simulator issues:** The build script auto-disables conflicting architectures. Use `-x` for xcframework builds to get all arch variants
