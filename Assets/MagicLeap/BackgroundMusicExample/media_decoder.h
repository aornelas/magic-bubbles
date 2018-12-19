// %BANNER_BEGIN%
// ---------------------------------------------------------------------
// %COPYRIGHT_BEGIN%
//
// Copyright (c) 2018 Magic Leap, Inc. (COMPANY) All Rights Reserved.
// Magic Leap, Inc. Confidential and Proprietary
//
// NOTICE:  All information contained herein is, and remains the property
// of COMPANY. The intellectual and technical concepts contained herein
// are proprietary to COMPANY and may be covered by U.S. and Foreign
// Patents, patents in process, and are protected by trade secret or
// copyright law.  Dissemination of this information or reproduction of
// this material is strictly forbidden unless prior written permission is
// obtained from COMPANY.  Access to the source code contained herein is
// hereby forbidden to anyone except current COMPANY employees, managers
// or contractors who have executed Confidentiality and Non-disclosure
// agreements explicitly covering such access.
//
// The copyright notice above does not evidence any actual or intended
// publication or disclosure  of  this source code, which includes
// information that is confidential and/or proprietary, and is a trade
// secret, of  COMPANY.   ANY REPRODUCTION, MODIFICATION, DISTRIBUTION,
// PUBLIC  PERFORMANCE, OR PUBLIC DISPLAY OF OR THROUGH USE  OF THIS
// SOURCE CODE  WITHOUT THE EXPRESS WRITTEN CONSENT OF COMPANY IS
// STRICTLY PROHIBITED, AND IN VIOLATION OF APPLICABLE LAWS AND
// INTERNATIONAL TREATIES.  THE RECEIPT OR POSSESSION OF  THIS SOURCE
// CODE AND/OR RELATED INFORMATION DOES NOT CONVEY OR IMPLY ANY RIGHTS
// TO REPRODUCE, DISCLOSE OR DISTRIBUTE ITS CONTENTS, OR TO MANUFACTURE,
// USE, OR SELL ANYTHING THAT IT  MAY DESCRIBE, IN WHOLE OR IN PART.
//
// %COPYRIGHT_END%
// --------------------------------------------------------------------*/
// %BANNER_END%

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <ml_api.h>

typedef struct {
    MLHandle    hExtractor;            // extractor for source (demuxer)
    MLHandle    hFormat;               // track format
    MLHandle    hCodec;                // track codec
    int64_t     track_samples_demuxed; // number of samples demuxed for the track
    bool        track_samples_end;     // samples demuxed for the track ended
    int64_t     track_frames_decoded;  // number of frames decoded for the track
    bool        track_frames_end;      // frames  decoded for the track ended

    int64_t     position_us;           // pts of the last decoded frame in microseconds
    int64_t     duration_us;           // format duration in microseconds

} DecoderContext;

MLResult AudioOpen(
        DecoderContext** ppDecoderContext,
        const char* szMediaUri,
        const char* szTrackType);

MLResult AudioProcessInput(
        DecoderContext* pDecoderContext,
        int32_t* p_samples_demuxed);

MLResult AudioProcessOutput(
        DecoderContext* pDecoderContext,
        int32_t* p_frames_decoded,
        void* context, // context pointer to be delivered to callback functions
        MLResult (pf_on_audio_format_changed)(void* context, int32_t sample_rate, int32_t channel_count),
        MLResult (pf_on_audio_frame_decoded) (void* context, const uint8_t* pbBuffer, size_t cbBuffer, int64_t pts) );

MLResult AudioFlush(
        DecoderContext* pDecoderContext);

MLResult AudioSeek(
        DecoderContext* pDecoderContext,
        int64_t time_us);

MLResult AudioGetPosition(
        DecoderContext* pDecoderContext,
        int64_t* p_position_us);

MLResult AudioGetDuration(
        DecoderContext* pDecoderContext,
        int64_t* p_duration_us);

MLResult AudioGetDecodeCompleted(
        DecoderContext* pDecoderContext,
        bool* p_decode_completed);

MLResult AudioRelease(
        DecoderContext* pDecoderContext);

#ifdef __cplusplus
}
#endif
