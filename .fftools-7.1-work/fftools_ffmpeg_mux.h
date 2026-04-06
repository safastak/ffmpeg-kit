/*
 * Muxer internal APIs - should not be included outside of ffmpeg_mux*
 * Copyright (c) 2023 ARTHENICA LTD
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/*
 * This file is the modified version of ffmpeg_mux.h file living in ffmpeg source code under the fftools folder. We
 * manually update it each time we depend on a new ffmpeg version. Below you can see the list of changes applied
 * by us to develop ffmpeg-kit library.
 *
 * ffmpeg-kit changes by ARTHENICA LTD
 *
 * 07.2023
 * --------------------------------------------------------
 * - FFmpeg 6.0 changes migrated
 * - fftools header names updated
 * - want_sdp made thread-local
 * - EncStatsFile declaration migrated from ffmpeg_mux_init.c
 * - WARN_MULTIPLE_OPT_USAGE, MATCH_PER_STREAM_OPT, MATCH_PER_TYPE_OPT, SPECIFIER_OPT_FMT declarations migrated from
 *   ffmpeg.h
 * - ms_from_ost migrated to ffmpeg_mux.c
 */

#ifndef FFTOOLS_FFMPEG_MUX_H
#define FFTOOLS_FFMPEG_MUX_H

#include <stdatomic.h>
#include <stdint.h>

#include "fftools_ffmpeg_sched.h"

#include "libavformat/avformat.h"

#include "libavcodec/packet.h"

#include "libavutil/dict.h"
#include "libavutil/fifo.h"

typedef struct EncStatsFile {
    char        *path;
    AVIOContext *io;
} EncStatsFile;

typedef struct MuxStream {
    OutputStream    ost;

    // name used for logging
    char            log_name[32];

    AVBSFContext   *bsf_ctx;
    AVPacket       *bsf_pkt;

    AVPacket       *pkt;

    EncStats        stats;

    int             sch_idx;
    int             sch_idx_enc;
    int             sch_idx_src;

    int             sq_idx_mux;

    int64_t         max_frames;

    // timestamp from which the streamcopied streams should start,
    // in AV_TIME_BASE_Q;
    // everything before it should be discarded
    int64_t         ts_copy_start;

    /* dts of the last packet sent to the muxer, in the stream timebase
     * used for making up missing dts values */
    int64_t         last_mux_dts;

    int64_t         stream_duration;
    AVRational      stream_duration_tb;

    // state for av_rescale_delta() call for audio in write_packet()
    int64_t         ts_rescale_delta_last;

    // combined size of all the packets sent to the muxer
    uint64_t        data_size_mux;

    int             copy_initial_nonkeyframes;
    int             copy_prior_start;
    int             streamcopy_started;
#if FFMPEG_OPT_VSYNC_DROP
    int             ts_drop;
#endif

    const char     *apad;
} MuxStream;

typedef struct Muxer {
    OutputFile              of;

    // name used for logging
    char                    log_name[32];

    AVFormatContext        *fc;

    Scheduler              *sch;
    unsigned                sch_idx;

    // OutputStream indices indexed by scheduler stream indices
    int                    *sch_stream_idx;
    int                  nb_sch_stream_idx;

    AVDictionary           *opts;

    // used to validate that all encoder avoptions have been actually used
    AVDictionary           *enc_opts_used;

    /* filesize limit expressed in bytes */
    int64_t                 limit_filesize;
    atomic_int_least64_t    last_filesize;
    int                     header_written;

    SyncQueue              *sq_mux;
    AVPacket               *sq_pkt;
} Muxer;

int mux_check_init(void *arg);

MuxStream *ms_from_ost(OutputStream *ost);

#endif /* FFTOOLS_FFMPEG_MUX_H */
