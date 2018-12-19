// %BANNER_BEGIN%
// ---------------------------------------------------------------------
// %COPYRIGHT_BEGIN%
//
// Copyright (c) 2018 Magic Leap, Inc. (COMPANY) All Rights Reserved.
// Magic Leap, Inc. Confidential and Proprietary
//
// NOTICE: All information contained herein is, and remains the property
// of COMPANY. The intellectual and technical concepts contained herein
// are proprietary to COMPANY and may be covered by U.S. and Foreign
// Patents, patents in process, and are protected by trade secstatus or
// copyright law. Dissemination of this information or reproduction of
// this material is strictly forbidden unless prior written permission is
// obtained from COMPANY. Access to the source code contained herein is
// hereby forbidden to anyone except current COMPANY employees, managers
// or contractors who have executed Confidentiality and Non-disclosure
// agreements explicitly covering such access.
//
// The copyright notice above does not evidence any actual or intended
// publication or disclosure of this source code, which includes
// information that is confidential and/or proprietary, and is a trade
// secret, of COMPANY. ANY REPRODUCTION, MODIFICATION, DISTRIBUTION,
// PUBLIC PERFORMANCE, OR PUBLIC DISPLAY OF OR THROUGH USE OF THIS
// SOURCE CODE WITHOUT THE EXPRESS WRITTEN CONSENT OF COMPANY IS
// STRICTLY PROHIBITED, AND IN VIOLATION OF APPLICABLE LAWS AND
// INTERNATIONAL TREATIES. THE RECEIPT OR POSSESSION OF THIS SOURCE
// CODE AND/OR RELATED INFORMATION DOES NOT CONVEY OR IMPLY ANY RIGHTS
// TO REPRODUCE, DISCLOSE OR DISTRIBUTE ITS CONTENTS, OR TO MANUFACTURE,
// USE, OR SELL ANYTHING THAT IT MAY DESCRIBE, IN WHOLE OR IN PART.
//
// %COPYRIGHT_END%
// --------------------------------------------------------------------
// %BANNER_END%

#include "media_decoder.h"
#include <string.h>

#include <stdlib.h>
#include <ml_logging.h>
#include <ml_media_format.h>
#include <ml_media_extractor.h>
#include <ml_media_codec.h>
#include <ml_media_error.h>

MLResult AudioOpen(
        DecoderContext** ppDecoderContext,
        const char* szMediaUri,
        const char* szTrackType)
{
    if( NULL == ppDecoderContext )
    {
        ML_LOG(Error, "example_music_provider.AudioOpen failed. Reason: ppDecoderContext was null.");
        return MLResult_InvalidParam;
    }

    DecoderContext* pDecoderContext = malloc(sizeof(DecoderContext));
    if( NULL == pDecoderContext )
    {
        ML_LOG(Error, "example_music_provider.AudioOpen failed. Reason: memory allocation failed.");
        return MLResult_AllocFailed;
    }

    pDecoderContext->hExtractor            = ML_INVALID_HANDLE;
    pDecoderContext->hFormat               = ML_INVALID_HANDLE;
    pDecoderContext->hCodec                = ML_INVALID_HANDLE;
    pDecoderContext->track_samples_demuxed =     0;
    pDecoderContext->track_samples_end     = false;
    pDecoderContext->track_frames_decoded  =     0;
    pDecoderContext->track_frames_end      = false;
    pDecoderContext->position_us           =     0;
    pDecoderContext->duration_us           =    -1;

    MLResult result = MLMediaExtractorCreate(&pDecoderContext->hExtractor);
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.AudioOpen call to MLMediaExtractorCreate failed. Reason: %s.", MLMediaResultGetString(result));
        if( pDecoderContext != NULL)
        {
            AudioRelease(pDecoderContext);
            pDecoderContext = NULL;
        }
        return result;
    }

    result = MLMediaExtractorSetDataSourceForPath(pDecoderContext->hExtractor, szMediaUri);
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.AudioOpen call to MLMediaExtractorSetDataSourceForPath failed. Reason: %s.", MLMediaResultGetString(result));
        if( pDecoderContext != NULL)
        {
            AudioRelease(pDecoderContext);
            pDecoderContext = NULL;
        }
        return result;
    }

    //
    // select the first track that matches the desired track type
    //

    uint64_t track_count = 0;
    result = MLMediaExtractorGetTrackCount(pDecoderContext->hExtractor, &track_count);
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.AudioOpen call to MLMediaExtractorGetTrackCount failed. Reason: %s.", MLMediaResultGetString(result));
        if( pDecoderContext != NULL)
        {
            AudioRelease(pDecoderContext);
            pDecoderContext = NULL;
        }
        return result;
    }

    ML_LOG(Info, "source: \"%s\" track_count: %ju\n", szMediaUri, track_count);

    char szMimeType[MAX_KEY_STRING_SIZE] = "";

    size_t track_index = 0;
    for( ; track_index < track_count; track_index++)
    {
        MLHandle hFormatTemp = ML_INVALID_HANDLE;

        result = MLMediaExtractorGetTrackFormat( pDecoderContext->hExtractor, track_index, &hFormatTemp );
        if( MLResult_Ok != result )
        {
            ML_LOG(Error, "example_music_provider.AudioOpen call to MLMediaExtractorGetTrackFormat failed. Reason: %s.", MLMediaResultGetString(result));
            if( pDecoderContext != NULL)
            {
                AudioRelease(pDecoderContext);
                pDecoderContext = NULL;
            }
            return result;
        }

        result = MLMediaFormatGetKeyString(hFormatTemp, MLMediaFormat_Key_Mime, szMimeType);
        if( MLResult_Ok != result )
        {
            ML_LOG(Error, "example_music_provider.AudioOpen call to MLMediaFormatGetKeyString failed. Reason: %s.", MLMediaResultGetString(result));
            if( pDecoderContext != NULL)
            {
                AudioRelease(pDecoderContext);
                pDecoderContext = NULL;
            }
            return result;
        }

        char szFormat[MAX_FORMAT_STRING_SIZE] = "";
        result = MLMediaFormatObjectToString(hFormatTemp, szFormat);
        if( MLResult_Ok != result )
        {
            ML_LOG(Error, "example_music_provider.AudioOpen call to MLMediaFormatObjectToString failed. Reason: %s.", MLMediaResultGetString(result));
            if( pDecoderContext != NULL)
            {
                AudioRelease(pDecoderContext);
                pDecoderContext = NULL;
            }
            return result;
        }

        ML_LOG(Info, "track_index: %zu mime_type: \"%s\" format: \"%s\"\n",
                track_index,
                szMimeType,
                szFormat);

        if( 0 == strncmp(szMimeType, szTrackType, strlen(szTrackType)) )
        {
            pDecoderContext->hFormat = hFormatTemp; // save the selected track's format
            break;
        }

        if( ML_INVALID_HANDLE != hFormatTemp )
        {
            MLMediaFormatDestroy(hFormatTemp);
            hFormatTemp = ML_INVALID_HANDLE;
        }
    }

    if( track_index >= track_count )
    {
        ML_LOG(Info, "error! no track starting with mime_type: \"%s\" found in media source: \"%s\"\n", szTrackType, szMediaUri);
        result = MLResult_InvalidParam;
        if( pDecoderContext != NULL)
        {
            AudioRelease(pDecoderContext);
            pDecoderContext = NULL;
        }
        return result;
    }

    result = MLMediaExtractorSelectTrack( pDecoderContext->hExtractor, track_index);
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.AudioOpen call to MLMediaExtractorSelectTrack failed. Reason: %s.", MLMediaResultGetString(result));
        if( pDecoderContext != NULL)
        {
            AudioRelease(pDecoderContext);
            pDecoderContext = NULL;
        }
        return result;
    }

    result = MLMediaFormatGetKeyValueInt64(pDecoderContext->hFormat, MLMediaFormat_Key_Duration, &(pDecoderContext->duration_us) );
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.AudioOpen call to MLMediaFormatGetKeyValueInt64 failed. Reason: %s.", MLMediaResultGetString(result));
        if( pDecoderContext != NULL)
        {
            AudioRelease(pDecoderContext);
            pDecoderContext = NULL;
        }
        return result;
    }

    result = MLMediaCodecCreateCodec(MLMediaCodecCreation_ByType, MLMediaCodecType_Decoder, szMimeType, &(pDecoderContext->hCodec));
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.AudioOpen call to MLMediaCodecCreateCodec failed. Reason: %s.", MLMediaResultGetString(result));
        if( pDecoderContext != NULL)
        {
            AudioRelease(pDecoderContext);
            pDecoderContext = NULL;
        }
        return result;
    }

    result = MLMediaCodecConfigure(pDecoderContext->hCodec, pDecoderContext->hFormat, (MLHandle)0);
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.AudioOpen call to MLMediaCodecConfigure failed. Reason: %s.", MLMediaResultGetString(result));
        if( pDecoderContext != NULL)
        {
            AudioRelease(pDecoderContext);
            pDecoderContext = NULL;
        }
        return result;
    }

    /*
    CHECK_ML( MLMediaCodecGetOutputFormat(pDecoderContext->codec, &pDecoderContext->format));
    char* newFormat = (char *)malloc(MAX_FORMAT_STRING_SIZE);
    if (newFormat == NULL) {
        ML_LOG(Error, "Failed to allocate memory for new format\n");
        result = MLResult_UnspecifiedFailure;
        goto done;
    }
    CHECK_ML( MLMediaFormatObjectToString(pDecoderContext->format, newFormat));
    ML_LOG(Info, "Format changed to: %s\n", newFormat);
    free(newFormat);

    // TODO: changing sampling rate on-the-fly is not supported by Audio Service.
    //       we can still "fake" this by changing the "pitch" of the stream.
    int32_t sampleRate = 0;
    int32_t channelCount = 0;
    CHECK_ML( MLMediaFormatGetKeyValueInt32(pDecoderContext->format, MLMediaFormat_Key_Sample_Rate, &sampleRate));
    CHECK_ML( MLMediaFormatGetKeyValueInt32(pDecoderContext->format, MLMediaFormat_Key_Channel_Count, &channelCount));
    CHECK_ML( MLMusicServiceProviderSetAudioOutput(sampleRate, channelCount));
    */

    result = MLMediaCodecStart(pDecoderContext->hCodec);
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.AudioOpen call to MLMediaCodecStart failed. Reason: %s.", MLMediaResultGetString(result));
        if( pDecoderContext != NULL)
        {
            AudioRelease(pDecoderContext);
            pDecoderContext = NULL;
        }
        return result;
    }

    *ppDecoderContext = pDecoderContext;

    ML_LOG(Info, "%s() line: %d media_source: \"%s\" returned: %s",
            __FUNCTION__, __LINE__, szMediaUri, MLMediaResultGetString(result) );

    return result;
}

MLResult AudioProcessInput(
        DecoderContext* pDecoderContext,
        int32_t* p_samples_demuxed)
{
    if( NULL == pDecoderContext )
    {
        ML_LOG(Error, "example_music_provider.AudioProcessInput failed. Reason: pDecoderContext was null.");
        return MLResult_InvalidParam;
    }

    if( !(pDecoderContext->hExtractor != ML_INVALID_HANDLE) )
    {
        ML_LOG(Error, "example_music_provider.AudioProcessInput failed. Reason: invalid extractor handle.");
        return MLResult_InvalidParam;
    }

    if( !(pDecoderContext->hCodec != ML_INVALID_HANDLE) )
    {
        ML_LOG(Error, "example_music_provider.AudioProcessInput failed. Reason: invalid codec handle.");
        return MLResult_InvalidParam;
    }

    if( p_samples_demuxed != NULL)
    {
        *p_samples_demuxed = 0;
    }

    // demuxing ended, nothing to do
    if( pDecoderContext->track_samples_end )
    {
        return MLResult_Ok;
    }

    //note: no need to return error, retry or wait at all
    //      - quite ordinary to have no input buffer available until a decode completes
    //      - no matter how long you wait or how many times you retry, until:
    //      - a sample is decoded and you dequeue it; this will not succeed

    int64_t  buffer_index = MLMediaCodec_TryAgainLater;
    uint8_t* pbBuffer = NULL;
    size_t   cbBuffer = 0;

    MLResult result = MLMediaCodecDequeueInputBuffer( pDecoderContext->hCodec, 0, &buffer_index);
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.AudioProcessInput call to MLMediaCodecDequeueInputBuffer failed. Reason: %s.", MLMediaResultGetString(result));
        return result;
    }

    if( buffer_index == MLMediaCodec_TryAgainLater)
    {
        return result;
    }

    result = MLMediaCodecGetInputBufferPointer( pDecoderContext->hCodec, (MLHandle)buffer_index, &pbBuffer, &cbBuffer);
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.AudioProcessInput call to MLMediaCodecGetInputBufferPointer failed. Reason: %s.", MLMediaResultGetString(result));
        return result;
    }

    if (pbBuffer == NULL)
    {
        ML_LOG(Error, "example_music_provider.AudioProcessInput failed. Reason: unspecified failure.");
        return MLResult_UnspecifiedFailure;
    }

    if (!(cbBuffer > 0))
    {
        ML_LOG(Error, "example_music_provider.AudioProcessInput failed. Reason: unspecified failure.");
        return MLResult_UnspecifiedFailure;
    }

    int64_t sample_size  = -1;  // -1 if no more samples are available

    result = MLMediaExtractorReadSampleData(pDecoderContext->hExtractor, pbBuffer, cbBuffer, 0, &sample_size);
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.AudioProcessInput call to MLMediaExtractorReadSampleData failed. Reason: %s.", MLMediaResultGetString(result));
        return result;
    }

    if( sample_size < 0 ) {
        ML_LOG(Info, "track end of input samples marked by sample_size: %jd\n", sample_size);
        pDecoderContext->track_samples_end = true;

        // queue an empty buffer marked EOS to denote the end of demuxing - otherwise infinite loop
        result = MLMediaCodecQueueInputBuffer( pDecoderContext->hCodec, (MLHandle)buffer_index, 0, 0, 0, MLMediaCodecBufferFlag_EOS );
        if( MLResult_Ok != result )
        {
            ML_LOG(Error, "example_music_provider.AudioProcessInput call to MLMediaCodecQueueInputBuffer failed. Reason: %s.", MLMediaResultGetString(result));
            return result;
        }
        return result;
    }

    int64_t sample_track = -1;  // important when we process multiple tracks like audio, video, subtitle
    int64_t sample_pts   = -1;  // -1 if no more samples are available
    int     sample_flags =  0;  // sample_flags & MLMediaCodecBufferFlag_EOS

    result = MLMediaExtractorGetSampleTrackIndex( pDecoderContext->hExtractor, &sample_track);
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.AudioProcessInput call to MLMediaExtractorGetSampleTrackIndex failed. Reason: %s.", MLMediaResultGetString(result));
        return result;
    }

    result = MLMediaExtractorGetSampleTime( pDecoderContext->hExtractor, &sample_pts);
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.AudioProcessInput call to MLMediaExtractorGetSampleTime failed. Reason: %s.", MLMediaResultGetString(result));
        return result;
    }

    result = MLMediaExtractorGetSampleFlags( pDecoderContext->hExtractor, &sample_flags);
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.AudioProcessInput call to MLMediaExtractorGetSampleFlags failed. Reason: %s.", MLMediaResultGetString(result));
        return result;
    }

    /*
    ML_LOG(Info, "track: %02jd demuxed sample: %06jd size: %6jd pts: %8jd flags:%c %c %c\n",
                sample_track,
                pDecoderContext->track_samples_demuxed,
                sample_size,
                sample_pts,
                (sample_flags & MLMediaCodecBufferFlag_KeyFrame)    ? 'K' : ' ',
                (sample_flags & MLMediaCodecBufferFlag_CodecConfig) ? 'C' : ' ',
                (sample_flags & MLMediaCodecBufferFlag_EOS)         ? 'E' : ' '  );
    */
    pDecoderContext->track_samples_demuxed += 1;

    if( p_samples_demuxed != NULL)
    {
        *p_samples_demuxed += 1;
    }

    result = MLMediaCodecQueueInputBuffer( pDecoderContext->hCodec, (MLHandle)buffer_index, 0, (size_t)sample_size, (uint64_t) sample_pts, sample_flags );
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.AudioProcessInput call to MLMediaCodecQueueInputBuffer failed. Reason: %s.", MLMediaResultGetString(result));
        return result;
    }

    result = MLMediaExtractorAdvance( pDecoderContext->hExtractor);
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.AudioProcessInput call to MLMediaExtractorAdvance failed. Reason: %s.", MLMediaResultGetString(result));
        return result;
    }

    return MLResult_Ok;
}

MLResult AudioProcessOutput(
        DecoderContext* pDecoderContext,
        int32_t* p_frames_decoded,
        void* context, // context pointer to be delivered to callback functions
        MLResult (pf_on_audio_format_changed)(void* context, int32_t sample_rate, int32_t channel_count),
        MLResult (pf_on_audio_frame_decoded) (void* context, const uint8_t* pbBuffer, size_t cbBuffer, int64_t pts) )
{

    int64_t buffer_index = MLMediaCodec_TryAgainLater;
    MLMediaCodecBufferInfo buffer_info = {0};
    MLResult result = MLResult_Ok;

    if( NULL == pDecoderContext )
    {
        ML_LOG(Error, "example_music_provider.AudioProcessOutput failed. Reason: pDecoderContext was null.");
        return MLResult_InvalidParam;
    }

    if( pDecoderContext->hCodec == ML_INVALID_HANDLE )
    {
        ML_LOG(Error, "example_music_provider.AudioProcessOutput failed. Reason: invalid codec handle.");
        return MLResult_InvalidParam;
    }

    if( p_frames_decoded != NULL)
    {
        *p_frames_decoded = 0;
    }

    // decoding ended, nothing to do
    if( pDecoderContext->track_frames_end )
    {
        return MLResult_Ok;
    }

    //note: no need to return error, retry or wait at all
    //      - quite ordinary to have no output buffer until decoding completes, no point in waiting
    //      - if you did not queue an input, you cannot dequeue an output no matter how long you wait
    //      - instead of waiting for dequeing, better to queue extract and queue an input buffer

    result = MLMediaCodecDequeueOutputBuffer(pDecoderContext->hCodec, &buffer_info, 0, &buffer_index);
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.AudioProcessOutput call to MLMediaCodecDequeueOutputBuffer failed. Reason: %s.", MLMediaResultGetString(result));
        if( buffer_index >= 0 )
        {
            MLMediaCodecReleaseOutputBuffer(pDecoderContext->hCodec, (MLHandle)buffer_index, false);
            buffer_index = MLMediaCodec_TryAgainLater;
        }
        return result;
    }

    if( buffer_index < 0) {
        if( buffer_index == MLMediaCodec_OutputBuffersChanged ) {
            ML_LOG(Info, "MLMediaCodec_OutputBuffersChanged"); // ? noone knows what this is
        }
        else if(buffer_index == MLMediaCodec_FormatChanged) {
            ML_LOG(Info, "MLMediaCodec_FormatChanged");

            MLHandle hCodecFormat = ML_INVALID_HANDLE;

            result = MLMediaCodecGetOutputFormat(pDecoderContext->hCodec, &hCodecFormat);
            if( MLResult_Ok != result )
            {
                ML_LOG(Error, "example_music_provider.AudioProcessOutput call to MLMediaCodecGetOutputFormat failed. Reason: %s.", MLMediaResultGetString(result));
                return result;
            }

            if( hCodecFormat == ML_INVALID_HANDLE )
            {
                ML_LOG(Error, "example_music_provider.AudioProcessOutput failed. Reason: invalid codec format.");
                return MLResult_UnspecifiedFailure;
            }

            // TODO: changing sampling rate on-the-fly is not supported by Audio Service.
            //       we can still "fake" this by changing the "pitch" of the stream.
            int32_t sample_rate  = 0;
            int32_t channel_count = 0;

            result = MLMediaFormatGetKeyValueInt32(hCodecFormat, MLMediaFormat_Key_Sample_Rate, &sample_rate);
            if( MLResult_Ok != result )
            {
                ML_LOG(Error, "example_music_provider.AudioProcessOutput call to MLMediaFormatGetKeyValueInt32 failed. Reason: %s.", MLMediaResultGetString(result));
                return result;
            }

            result = MLMediaFormatGetKeyValueInt32(hCodecFormat, MLMediaFormat_Key_Channel_Count, &channel_count);
            if( MLResult_Ok != result )
            {
                ML_LOG(Error, "example_music_provider.AudioProcessOutput call to MLMediaFormatGetKeyValueInt32 failed. Reason: %s.", MLMediaResultGetString(result));
                return result;
            }

            if( !(sample_rate > 0) )
            {
                ML_LOG(Error, "example_music_provider.AudioProcessOutput failed. Reason: invalid sample rate.");
                return MLResult_UnspecifiedFailure;
            }

            if( !(channel_count > 0) )
            {
                ML_LOG(Error, "example_music_provider.AudioProcessOutput failed. Reason: invalid channel count.");
                return MLResult_UnspecifiedFailure;
            }

            char* codec_format = (char*)malloc(MAX_FORMAT_STRING_SIZE);
            if( NULL != codec_format)
            {
                if( MLResult_Ok == MLMediaFormatObjectToString(hCodecFormat, codec_format) )
                {
                    ML_LOG(Info, "format changed to: %s", codec_format);
                }
                free(codec_format);
                codec_format = NULL;
            }

            if( pf_on_audio_format_changed != NULL)
            {
                result = (*pf_on_audio_format_changed)(context, sample_rate, channel_count);
                if( MLResult_Ok != result )
                {
                    ML_LOG(Error, "example_music_provider.AudioProcessOutput callback to audio format changed failed. Reason: %s.", MLMediaResultGetString(result));
                    return result;
                }
            }
        }
        // no decoded frames in output buffer queue for now
        return MLResult_Ok;
    }

    if( buffer_info.flags & MLMediaCodecBufferFlag_EOS )
    {
        ML_LOG(Info, "track end of output frames marked by eos flag\n");
        pDecoderContext->track_frames_end = true;
        if( buffer_info.size <= 0)
        {
            if( buffer_index >= 0 ) {
                MLMediaCodecReleaseOutputBuffer(pDecoderContext->hCodec, (MLHandle)buffer_index, false);
                buffer_index = MLMediaCodec_TryAgainLater;
            }
            return MLResult_Ok;
        }
    }

    /*
    ML_LOG(Info, "decoded frame: %08jd offset: %zu size: %zu pts: %10jd flags:%c%c%c\n",
                pDecoderContext->track_frames_decoded,
                buffer_info.offset,
                buffer_info.size,
                buffer_info.presentation_time_us,
                (buffer_info.flags & MLMediaCodecBufferFlag_KeyFrame)    ? 'K' : ' ',
                (buffer_info.flags & MLMediaCodecBufferFlag_CodecConfig) ? 'C' : ' ',
                (buffer_info.flags & MLMediaCodecBufferFlag_EOS)         ? 'E' : ' ' );
    */
    pDecoderContext->track_frames_decoded += 1;

    if( p_frames_decoded != NULL)
    {
        *p_frames_decoded += 1;
    }

    //process decoded buffer
    const uint8_t* pbBuffer = NULL;
    size_t         cbBuffer = 0;

    result = MLMediaCodecGetOutputBufferPointer(pDecoderContext->hCodec, (MLHandle) buffer_index, &pbBuffer, &cbBuffer);
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.AudioProcessOutput call to MLMediaCodecGetOutputBufferPointer failed. Reason: %s.", MLMediaResultGetString(result));
        if( buffer_index >= 0 )
        {
            MLMediaCodecReleaseOutputBuffer(pDecoderContext->hCodec, (MLHandle)buffer_index, false);
            buffer_index = MLMediaCodec_TryAgainLater;
        }
        return result;
    }

    if( NULL == pbBuffer )
    {
        ML_LOG(Error, "example_music_provider.AudioProcessOutput failed. Reason: pbBuffer is null.");
        if( buffer_index >= 0 )
        {
            MLMediaCodecReleaseOutputBuffer(pDecoderContext->hCodec, (MLHandle)buffer_index, false);
            buffer_index = MLMediaCodec_TryAgainLater;
        }
        return MLResult_UnspecifiedFailure;
    }

    if( !(cbBuffer > 0) )
    {
        ML_LOG(Error, "example_music_provider.AudioProcessOutput failed. Reason: invalid cbBuffer.");
        if( buffer_index >= 0 )
        {
            MLMediaCodecReleaseOutputBuffer(pDecoderContext->hCodec, (MLHandle)buffer_index, false);
            buffer_index = MLMediaCodec_TryAgainLater;
        }
        return MLResult_UnspecifiedFailure;
    }

    pDecoderContext->position_us = buffer_info.presentation_time_us;

    if( pf_on_audio_frame_decoded != NULL )
    {
        result = (*pf_on_audio_frame_decoded)(context, pbBuffer, cbBuffer, buffer_info.presentation_time_us);
        if( MLResult_Ok != result )
        {
            ML_LOG(Error, "example_music_provider.AudioProcessOutput callback to on audio frame decoded failed. Reason: %s.", MLMediaResultGetString(result));
            if( buffer_index >= 0 )
            {
                MLMediaCodecReleaseOutputBuffer(pDecoderContext->hCodec, (MLHandle)buffer_index, false);
                buffer_index = MLMediaCodec_TryAgainLater;
            }
            return result;
        }
    }

    if( buffer_index >= 0 ) {
        MLMediaCodecReleaseOutputBuffer(pDecoderContext->hCodec, (MLHandle)buffer_index, false);
        buffer_index = MLMediaCodec_TryAgainLater;
    }
    return result;
}

MLResult AudioSeek(
        DecoderContext* pContext,
        int64_t time_us) {

    MLResult result = MLResult_Ok;

    if( NULL == pContext )
    {
        ML_LOG(Error, "example_music_provider.AudioSeek failed. Reason: pContext is null.");
        return MLResult_InvalidParam;
    }

    if( pContext->hCodec == ML_INVALID_HANDLE )
    {
        ML_LOG(Error, "example_music_provider.AudioSeek failed. Reason: invalid codec.");
        return MLResult_InvalidParam;
    }

    if( pContext->hExtractor == ML_INVALID_HANDLE )
    {
        ML_LOG(Error, "example_music_provider.AudioSeek failed. Reason: invalid extractor.");
        return MLResult_InvalidParam;
    }

    result = MLMediaCodecFlush( pContext->hCodec);
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.AudioSeek call to MLMediaCodecFlush failed. Reason: %s.", MLMediaResultGetString(result));
        return result;
    }

    result = MLMediaExtractorSeekTo(pContext->hExtractor, time_us, MLMediaSeekMode_Next_Sync);
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.AudioSeek call to MLMediaExtractorSeekTo failed. Reason: %s.", MLMediaResultGetString(result));
        return result;
    }

    return result;
}

MLResult AudioFlush(DecoderContext* pDecoderContext)
{
    MLResult result = MLResult_Ok;

    if( NULL == pDecoderContext )
    {
        ML_LOG(Error, "example_music_provider.AudioFlush failed. Reason: pDecoderContext is null.");
        return MLResult_InvalidParam;
    }

    if( pDecoderContext->hCodec == ML_INVALID_HANDLE )
    {
        ML_LOG(Error, "example_music_provider.AudioFlush failed. Reason: invalid codec.");
        return MLResult_InvalidParam;
    }

    result = MLMediaCodecFlush(pDecoderContext->hCodec);
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.AudioFlush call to MLMediaCodecFlush failed. Reason: %s.", MLMediaResultGetString(result));
        return result;
    }

    return result;
}

MLResult AudioGetDuration(
        DecoderContext* pDecoderContext,
        int64_t* p_duration_us) {

    MLResult result = MLResult_Ok;

    if( NULL == pDecoderContext )
    {
        ML_LOG(Error, "example_music_provider.AudioGetDuration failed. Reason: pDecoderContext is null.");
        return MLResult_InvalidParam;
    }

    if( NULL == p_duration_us )
    {
        ML_LOG(Error, "example_music_provider.AudioGetDuration failed. Reason: p_duration_us is null.");
        return MLResult_InvalidParam;
    }

    *p_duration_us = pDecoderContext->duration_us;
    return result;
}

MLResult AudioGetPosition(
        DecoderContext* pDecoderContext,
        int64_t* p_position_us) {

    MLResult result = MLResult_Ok;

    if( NULL == pDecoderContext )
    {
        ML_LOG(Error, "example_music_provider.AudioGetPosition failed. Reason: pDecoderContext is null.");
        return MLResult_InvalidParam;
    }

    if( NULL == p_position_us )
    {
        ML_LOG(Error, "example_music_provider.AudioGetPosition failed. Reason: p_duration_us is null.");
        return MLResult_InvalidParam;
    }

    *p_position_us = pDecoderContext->position_us;
    return result;
}

MLResult AudioGetDecodeCompleted(
        DecoderContext* pDecoderContext,
        bool* p_decode_completed)
{

    MLResult result = MLResult_Ok;

    if( NULL == pDecoderContext )
    {
        ML_LOG(Error, "example_music_provider.AudioGetDecodeCompleted failed. Reason: pDecoderContext is null.");
        return MLResult_InvalidParam;
    }

    if( NULL == p_decode_completed )
    {
        ML_LOG(Error, "example_music_provider.AudioGetDecodeCompleted failed. Reason: p_decode_completed is null.");
        return MLResult_InvalidParam;
    }

    *p_decode_completed = pDecoderContext->track_frames_end;
    return result;
}

MLResult AudioRelease(DecoderContext* pDecoderContext)
{
    MLResult result = MLResult_Ok;

    if( NULL == pDecoderContext )
    {
        ML_LOG(Error, "example_music_provider.AudioRelease failed. Reason: pDecoderContext is null.");
        return MLResult_InvalidParam;
    }

    if( ML_INVALID_HANDLE != pDecoderContext->hCodec )
    {
        result = MLMediaCodecFlush(pDecoderContext->hCodec);
        if( MLResult_Ok != result )
        {
            ML_LOG(Error, "example_music_provider.AudioRelease call to MLMediaCodecFlush failed. Reason: %s.", MLMediaResultGetString(result));
            if( NULL != pDecoderContext)
            {
                free( pDecoderContext );
                pDecoderContext = NULL;
            }
            return result;
        }

        result = MLMediaCodecStop(pDecoderContext->hCodec);
        if( MLResult_Ok != result )
        {
            ML_LOG(Error, "example_music_provider.AudioRelease call to MLMediaCodecStop failed. Reason: %s.", MLMediaResultGetString(result));
            if( NULL != pDecoderContext)
            {
                free( pDecoderContext );
                pDecoderContext = NULL;
            }
            return result;
        }

        result = MLMediaCodecDestroy(pDecoderContext->hCodec);
        if( MLResult_Ok != result )
        {
            ML_LOG(Error, "example_music_provider.AudioRelease call to MLMediaCodecDestroy failed. Reason: %s.", MLMediaResultGetString(result));
            if( NULL != pDecoderContext)
            {
                free( pDecoderContext );
                pDecoderContext = NULL;
            }
            return result;
        }
        pDecoderContext->hCodec = ML_INVALID_HANDLE;
    }
    if( ML_INVALID_HANDLE != pDecoderContext->hFormat )
    {
        result = MLMediaFormatDestroy(pDecoderContext->hFormat);
        if( MLResult_Ok != result )
        {
            ML_LOG(Error, "example_music_provider.AudioRelease call to MLMediaFormatDestroy failed. Reason: %s.", MLMediaResultGetString(result));
            if( NULL != pDecoderContext)
            {
                free( pDecoderContext );
                pDecoderContext = NULL;
            }
            return result;
        }
        pDecoderContext->hFormat = ML_INVALID_HANDLE;
    }
    if( ML_INVALID_HANDLE != pDecoderContext->hExtractor)
    {
        result = MLMediaExtractorDestroy(pDecoderContext->hExtractor);
        if( MLResult_Ok != result )
        {
            ML_LOG(Error, "example_music_provider.AudioRelease call to MLMediaExtractorDestroy failed. Reason: %s.", MLMediaResultGetString(result));
            if( NULL != pDecoderContext)
            {
                free( pDecoderContext );
                pDecoderContext = NULL;
            }
            return result;
        }
        pDecoderContext->hExtractor = ML_INVALID_HANDLE;
    }
    pDecoderContext->track_samples_demuxed = 0;
    pDecoderContext->track_samples_end     = false;
    pDecoderContext->track_frames_decoded  = 0;
    pDecoderContext->track_frames_end      = false;
    pDecoderContext->position_us           = -1;
    pDecoderContext->duration_us           = -1;

    if( NULL != pDecoderContext)
    {
        free( pDecoderContext );
        pDecoderContext = NULL;
    }
    return result;
}
