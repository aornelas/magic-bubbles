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

#define ML_DEFAULT_LOG_TAG "ExampleMusicProvider"

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#include <ml_logging.h>
#include <ml_lifecycle.h>
#include <ml_media_error.h>
#include <ml_music_service_provider.h>

#include "utility.h"
#include "media_decoder.h"

// tells event loop running on a different thread what to do next
// bms callbacks receive user commands on a different thread
// bms callbacks specify what arguments is to be made in context structure

typedef enum {
    COMMAND_NONE = 0,
    COMMAND_OPEN,
    COMMAND_START,
    COMMAND_STOP,
    COMMAND_PAUSE,
    COMMAND_RESUME,
    COMMAND_SEEK,

    // playlist related commands
    COMMAND_NEXT,
    COMMAND_PREVIOUS,

    COMMAND_EXIT,
} Command;

// Explicitly * 4 to account for 4 bytes max needed per UTF-8 character
#define URI_LENGTH_MAX  (1024 * 4)

typedef struct {
    char**   uris;      // playlist array of uris
    int32_t  size;      // playlist size
    int32_t  position;  // index of currently playing uri
} Playlist;

typedef struct {
    Command                     command;
    MLMusicServicePlaybackState playbackState;
    DecoderContext*             pDecoderContext; // contains internals of the decoder

    char                        mediaUri[URI_LENGTH_MAX];   // uri of currently playing track

    MLMusicServiceRepeatState   repeatState;
    MLMusicServiceShuffleState  shuffleState;

    //used to pass command arguments from BMS callbacks to main thread
    char                        desiredUri[URI_LENGTH_MAX]; // arg for COMMAND_OPEN
    uint32_t                    desiredSeekPosition;        // arg for COMMAND_SEEK

    Playlist                    playlist;
} Context;

// N/A for BMS Demo Player
MLResult BMSProviderSetAuthString(const char* auth_string, void* context)
{
    ML_LOG(Info, "%s() line: %d auth_string: \"%s\" context: %p", __FUNCTION__, __LINE__, auth_string ? auth_string : "", context);
    return MLResult_Ok;
}

MLResult PlaylistAppend(Playlist* pPlaylist, const char* uri_path)
{
    if( NULL == pPlaylist )
    {
        ML_LOG(Error, "example_music_provider.PlaylistAppend failed. Reason: pPlaylist was null.");
        return MLResult_InvalidParam;
    }

    if( NULL == uri_path )
    {
        ML_LOG(Error, "example_music_provider.PlaylistAppend failed. Reason: uri_path was null.");
        return MLResult_InvalidParam;
    }

    pPlaylist->uris = (char**)realloc(pPlaylist->uris, (pPlaylist->size + 1) * sizeof(char*));
    if( NULL == pPlaylist->uris )
    {
        ML_LOG(Error, "example_music_provider.PlaylistAppend failed. Reason: MLResult_AllocFailed.");
        return MLResult_AllocFailed;
    }

    size_t cbSize = strlen(uri_path)+1;
    pPlaylist->uris[pPlaylist->size] = (char*) malloc( cbSize );
    if( NULL == pPlaylist->uris[pPlaylist->size] )
    {
        ML_LOG(Error, "example_music_provider.PlaylistAppend failed. Reason: MLResult_AllocFailed.");
        return MLResult_AllocFailed;
    }

    strncpy ( pPlaylist->uris[pPlaylist->size], uri_path, cbSize );
    pPlaylist->size += 1;

    return MLResult_Ok;
}

MLResult BMSProviderSetUrl(const char* uri, void* context)
{
    ML_LOG(Info, "%s() line: %d uri: %s context: %p", __FUNCTION__, __LINE__, uri ? uri : "", context);
    MLResult result = MLResult_Ok;

    if( NULL == uri )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderSetUrl failed. Reason: uri was null.");
        return MLResult_InvalidParam;
    }

    if( NULL == context )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderSetUrl failed. Reason: context was null.");
        return MLResult_InvalidParam;
    }
    Context* pContext = (Context*)context;

    // only add a string uri to the playlist after confirming its existence
    char uri_path[URI_LENGTH_MAX] = "";
    result = SearchMedia(uri, uri_path, sizeof(uri_path));
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderSetUrl call to SearchMedia failed. Reason: %s.", MLMediaResultGetString(result));
        return result;
    }

    // append uri to the bottom of playlist
    int32_t position = pContext->playlist.size;
    result = PlaylistAppend( &pContext->playlist, uri_path);
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderSetUrl call to PlaylistAppend failed. Reason: %s.", MLMediaResultGetString(result));
        return result;
    }

    pContext->playlist.position = position;

    ML_LOG(Info, "%s() line: %d playlist.uris[playlist.position]: %s", __FUNCTION__, __LINE__,
            pContext->playlist.uris[pContext->playlist.position]);

    strncpy( pContext->desiredUri, pContext->playlist.uris[pContext->playlist.position], sizeof(pContext->desiredUri));

    pContext->command = COMMAND_OPEN;

    return result;
}

MLResult BMSProviderSetPlaylist(const char** list, size_t count, void* context)
{
    if( NULL == context )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderSetPlaylist failed. Reason: context was null.");
        return MLResult_InvalidParam;
    }
    Context* pContext = (Context*)context;

    if( NULL == list )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderSetPlaylist failed. Reason: list was null.");
        return MLResult_InvalidParam;
    }

    if( !(count > 0) )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderSetPlaylist failed. Reason: count is not greather than 0.");
        return MLResult_InvalidParam;
    }

    int32_t position = pContext->playlist.size;
    for( size_t i = 0; i < count; ++i) {

        ML_LOG(Info, "%s() line: %d uri: %s", __FUNCTION__, __LINE__, list[i] ? list[i] : "");
        if( list[i] == NULL || strlen(list[i]) <= 0) {
            continue;
        }

        // only add a uri to the playlist after confirming its existence
        char uri_path[URI_LENGTH_MAX] = "";
        if( MLResult_Ok == SearchMedia(list[i], uri_path, sizeof(uri_path)) &&
            strlen(uri_path) > 0) {
            PlaylistAppend( &pContext->playlist, uri_path);
        }
    }
    pContext->playlist.position = position;

    ML_LOG(Info, "%s() line: %d playlist.uris[playlist.position]: %s", __FUNCTION__, __LINE__,
            pContext->playlist.uris[pContext->playlist.position]);

    strncpy( pContext->desiredUri, pContext->playlist.uris[pContext->playlist.position], sizeof(pContext->desiredUri));

    pContext->command = COMMAND_OPEN;

    return MLResult_Ok;
}

MLResult BMSProviderStart(void* context)
{
    if( NULL == context )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderStart failed. Reason: context was null.");
        return MLResult_InvalidParam;
    }
    Context* pContext = (Context*)context;

    pContext->command = COMMAND_START;

    return MLResult_Ok;
}

MLResult BMSProviderStop(void* context)
{
    if( NULL == context )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderStop failed. Reason: context was null.");
        return MLResult_InvalidParam;
    }
    Context* pContext = (Context*)context;

    pContext->command = COMMAND_STOP;

    return MLResult_Ok;
}

MLResult BMSProviderPause(void* context)
{
    if( NULL == context )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderPause failed. Reason: context was null.");
        return MLResult_InvalidParam;
    }
    Context* pContext = (Context*)context;

    pContext->command = COMMAND_PAUSE;

    return MLResult_Ok;
}

MLResult BMSProviderResume(void* context)
{
    if( NULL == context )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderResume failed. Reason: context was null.");
        return MLResult_InvalidParam;
    }
    Context* pContext = (Context*)context;

    pContext->command = COMMAND_RESUME;

    return MLResult_Ok;
}

MLResult BMSProviderSeek(uint32_t seekPosition, void* context)
{
    if( NULL == context )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderSeek failed. Reason: context was null.");
        return MLResult_InvalidParam;
    }
    Context* pContext = (Context*)context;

    if (pContext->playbackState == MLMusicServicePlaybackState_Stopped)
    {
        MLResult result = BMSProviderStart(context);
        if (result != MLResult_Ok)
        {
            ML_LOG(Error, "example_music_provider.BMSProviderSeek failed, unable to restart codec after seeing previous end. Reason: %d", result);
            return result;
        }
    }

    pContext->desiredSeekPosition = seekPosition;
    pContext->command = COMMAND_SEEK;

    return MLResult_Ok;
}

MLResult BMSProviderNext(void* context)
{
    if( NULL == context )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderNext failed. Reason: context was null.");
        return MLResult_InvalidParam;
    }
    Context* pContext = (Context*)context;

    pContext->command = COMMAND_NEXT;

    return MLResult_Ok;
}

MLResult BMSProviderPrevious(void* context)
{
    if( NULL == context )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderPrevious failed. Reason: context was null.");
        return MLResult_InvalidParam;
    }
    Context* pContext = (Context*)context;

    pContext->command = COMMAND_PREVIOUS;

    return MLResult_Ok;
}

MLResult BMSProviderSetShuffle(MLMusicServiceShuffleState shuffleState, void* context)
{
    if( NULL == context )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderSetShuffle failed. Reason: context was null.");
        return MLResult_InvalidParam;
    }
    Context* pContext = (Context*)context;

    pContext->shuffleState = shuffleState;
    MLResult result = MLMusicServiceProviderNotifyShuffleStateChange(pContext->shuffleState);
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderSetShuffle call to MLMusicServiceProviderNotifyShuffleStateChange failed. Reason: %s.", MLMediaResultGetString(result));
        return result;
    }

    return MLResult_Ok;
}

MLResult BMSProviderSetRepeat(MLMusicServiceRepeatState repeatState, void* context)
{
    if( NULL == context )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderSetShuffle failed. Reason: context was null.");
        return MLResult_InvalidParam;
    }
    Context* pContext = (Context*)context;

    pContext->repeatState = repeatState;
    MLResult result = MLMusicServiceProviderNotifyRepeatStateChange(pContext->repeatState);
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderSetRepeat call to MLMusicServiceProviderNotifyRepeatStateChange failed. Reason: %s.", MLMediaResultGetString(result));
        return result;
    }

    return MLResult_Ok;
}

MLResult BMSProviderSetVolume(float volume, void* context)
{
    MLResult result = MLResult_Ok;

    result = MLMusicServiceProviderSetVolume( volume );
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderSetVolume call to MLMusicServiceProviderSetVolume failed. Reason: %s.", MLMediaResultGetString(result));
        return result;
    }

    result = MLMusicServiceProviderNotifyVolumeChange( volume );
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderSetVolume call to MLMusicServiceProviderNotifyVolumeChange failed. Reason: %s.", MLMediaResultGetString(result));
        return result;
    }

    return MLResult_Ok;
}

MLResult BMSProviderGetTrackLength(uint32_t* length, void* context)
{
    if( NULL == length )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderGetTrackLength failed. Reason: length was null.");
        return MLResult_InvalidParam;
    }

    if( NULL == context )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderGetTrackLength failed. Reason: context was null.");
        return MLResult_InvalidParam;
    }
    Context* pContext = (Context*)context;

    if( NULL == pContext->pDecoderContext )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderGetTrackLength failed. Reason: pContext->pDecoderContext was null.");
        return MLResult_InvalidParam;
    }

    int64_t duration_us = 0;
    MLResult result = AudioGetDuration( pContext->pDecoderContext, &duration_us);
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderGetTrackLength call to AudioGetDuration failed. Reason: %s.", MLMediaResultGetString(result));
        return result;
    }

    *length = duration_us / 1000000; // seconds resolution

    return MLResult_Ok;
}

MLResult BMSProviderGetCurrentPosition(uint32_t* position, void* context)
{
    if( NULL == position )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderGetCurrentPosition failed. Reason: position was null.");
        return MLResult_InvalidParam;
    }

    if( NULL == context )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderGetCurrentPosition failed. Reason: context was null.");
        return MLResult_InvalidParam;
    }
    Context* pContext = (Context*)context;

    if( NULL == pContext->pDecoderContext )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderGetCurrentPosition failed. Reason: pContext->pDecoderContext was null.");
        return MLResult_InvalidParam;
    }

    int64_t position_us = 0;
    MLResult result = AudioGetPosition( pContext->pDecoderContext, &position_us);
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderGetCurrentPosition call to AudioGetPosition failed. Reason: %s.", MLMediaResultGetString(result));
        return result;
    }

    *position = position_us / 1000;

    return MLResult_Ok;
}

// Unfortunately, ML C API (not the BMS API) does not yet retrieve metadata information from media sources
MLResult BMSProviderGetMetadata(MLMusicServiceTrackType track, MLMusicServiceMetadata* pMetadata, void* context)
{
    if( NULL == context )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderGetMetadata failed. Reason: context was null.");
        return MLResult_InvalidParam;
    }
    Context* pContext = (Context*)context;

    if( NULL == pContext->pDecoderContext )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderGetMetadata failed. Reason: pContext->pDecoderContext was null.");
        return MLResult_InvalidParam;
    }

    if( NULL == pMetadata )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderGetMetadata failed. Reason: pMetadata was null.");
        return MLResult_InvalidParam;
    }

    int64_t duration_us = 0;
    MLResult result = AudioGetDuration( pContext->pDecoderContext, &duration_us);
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderGetMetadata call to AudioGetDuration failed. Reason: %s.", MLMediaResultGetString(result));
        return result;
    }

    if( track != MLMusicServiceTrackType_Current )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderGetMetadata failed. Reason: track was not current.");
        return MLResult_InvalidParam;
    }

    // return the past after '/' to avoid returning full path ie; return a.mp3 rather than /package/resources/a.mp3
    char* pszTitle = strrchr( pContext->mediaUri, '/');
    if( pszTitle != NULL) {
        pszTitle += 1;
    }
    else {
        pszTitle = pContext->mediaUri;
    }

    pMetadata->track_title     = pszTitle;
    pMetadata->album_name      = u8"\xc2\xabNot Available In Example\xc2\xbb";
    pMetadata->album_url       = u8"\xc2\xabNot Available In Example\xc2\xbb";
    pMetadata->album_cover_url = u8"\xc2\xabNot Available In Example\xc2\xbb";
    pMetadata->artist_name     = u8"\xc2\xabNot Available In Example\xc2\xbb";
    pMetadata->artist_url      = u8"\xc2\xabNot Available In Example\xc2\xbb";
    pMetadata->length = duration_us / 1000000; // seconds resolution

    return MLResult_Ok;
}

void BMSProviderOnEndService(void* context)
{
    if( NULL == context )
    {
        ML_LOG(Error, "example_music_provider.BMSProviderOnEndService failed. Reason: context was null.");
        return;
    }
    Context* pContext = (Context*)context;

    pContext->command = COMMAND_EXIT;

    return;
}

// these functions are called on the same thread
MLResult OnAudioFormatChanged(void* context, int32_t sample_rate, int32_t channel_count)
{
    if( NULL == context )
    {
        ML_LOG(Error, "example_music_provider.OnAudioFormatChanged failed. Reason: context was null.");
        return MLResult_InvalidParam;
    }
    Context* pContext = (Context*)context;

    MLResult result = MLMusicServiceProviderSetAudioOutput(sample_rate, channel_count);
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.OnAudioFormatChanged call to MLMusicServiceProviderSetAudioOutput failed. Reason: %s.", MLMediaResultGetString(result));
        return result;
    }

    return MLResult_Ok;
}

MLResult OnAudioFrameDecoded(void* context, const uint8_t* pbBuffer, size_t cbBuffer, int64_t pts_us)
{
    if( NULL == context )
    {
        ML_LOG(Error, "example_music_provider.OnAudioFrameDecoded failed. Reason: context was null.");
        return MLResult_InvalidParam;
    }
    Context* pContext = (Context*)context;

    if( NULL == pbBuffer )
    {
        ML_LOG(Error, "example_music_provider.OnAudioFrameDecoded failed. Reason: pbBuffer was null.");
        return MLResult_InvalidParam;
    }

    if( !(cbBuffer > 0) )
    {
        ML_LOG(Error, "example_music_provider.OnAudioFrameDecoded failed. Reason: pbBuffer was null.");
        return MLResult_InvalidParam;
    }

    MLResult result = MLMusicServiceProviderWriteAudioOutput(pbBuffer, cbBuffer, true);
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.OnAudioFrameDecoded call to MLMusicServiceProviderWriteAudioOutput failed. Reason: %s.", MLMediaResultGetString(result));
        return result;
    }

    result = MLMusicServiceProviderNotifyPositionChange((uint32_t) pts_us / 1000);
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.OnAudioFrameDecoded call to MLMusicServiceProviderNotifyPositionChange failed. Reason: %s.", MLMediaResultGetString(result));
        return result;
    }

    return MLResult_Ok;
}

MLResult PlaylistPlayIndex(Context* pContext, int32_t position, bool start_playback)
{
    if( NULL == pContext )
    {
        ML_LOG(Error, "example_music_provider.PlaylistPlayIndex failed. Reason: pContext was null.");
        return MLResult_InvalidParam;
    }

    if( !(position >= 0 && position < pContext->playlist.size) )
    {
        ML_LOG(Error, "example_music_provider.PlaylistPlayIndex failed. Reason: invalid position.");
        return MLResult_InvalidParam;
    }

    if( NULL != pContext->pDecoderContext)
    {
        AudioRelease( pContext->pDecoderContext );
        pContext->pDecoderContext = NULL;
    }

    // stop playing the current track - sometimes it already ended
    if( pContext->playbackState != MLMusicServicePlaybackState_Stopped)
    {
        pContext->playbackState = MLMusicServicePlaybackState_Stopped;
        MLMusicServiceProviderNotifyPlaybackStateChange(pContext->playbackState);
    }

    pContext->playlist.position = position;
    if( start_playback )
    {
        strncpy( pContext->desiredUri, pContext->playlist.uris[pContext->playlist.position], sizeof(pContext->desiredUri));
        pContext->command = COMMAND_OPEN;
    }
    else
    {
        // set the next the next track to play when start is pressed, but don't start playing
        // eg;  when repeat_album is off and the album finishes playing;
        //      the next track to play is the top one rather than replaying the last one.
        strncpy( pContext->mediaUri, pContext->playlist.uris[pContext->playlist.position], sizeof(pContext->mediaUri));
    }

    return MLResult_Ok;
}

//tests
// 1- start playing a playlist: all tracks should play in order then fall to stopped state(because repeat_album is unset by default), then pressing play must start with the first track
// 2- start playing a playlist, set repeat_album: it should play the playlist then replay the playlist indefinitely
// 3- set repeat_song any moment and it should repeat that track again and again indefinitely(if you press next/prev or the track is completes by itself; it will still repeat the current song)
// 4- set shuffle on: each time a track is completed, a random song in the playlist will start playing (regardless of repeat_album or repeat_song)

// handle track transitions:
// this function is called in 3 ways
//      COMMAND_NONE     : track completed playing naturally
//      COMMAND_NEXT     : user pressed next
//      COMMAND_PREVIOUS : user pressed previous
// runs on ThreadMain
MLResult HandlePlaylist(Context* pContext, Command command)
{
    if( NULL == pContext )
    {
        ML_LOG(Error, "example_music_provider.HandlePlaylist failed. Reason: pContext was null.");
        return MLResult_InvalidParam;
    }

    // when repeat song is on; next/prev/track completion will all play the same track
    if( pContext->repeatState == MLMusicServiceRepeatState_Song)
    {
        ML_LOG(Info, "%s() line: %d RepeatState_Song track: \"%s\"", __FUNCTION__, __LINE__,
                pContext->playlist.uris[pContext->playlist.position]);

        int32_t position = pContext->playlist.position; // next track to play is the current one

        PlaylistPlayIndex(pContext, position, true);
    }
    else if( pContext->shuffleState == MLMusicServiceShuffleState_On)
    {
        // when shuffle is on; next/prev/track completion will start playing a randomly selected track
        // shuffle takes precedence over repeat_album
        // here we will make all of them behave the same since replicating exact VLC behavior would complicate code and gain little
        int32_t position = pContext->playlist.position;
        while( position == pContext->playlist.position )
        {
            srand(time(NULL));
            position = rand() % pContext->playlist.size;
        }

        ML_LOG(Info, "%s() line: %d shuffle_on: playing a random track: %d in playlist: \"%s\"",
                __FUNCTION__, __LINE__, position, pContext->playlist.uris[position]);

        PlaylistPlayIndex(pContext, position, true);
    }
    else
    {
         // if current track is not the last one in playlist; track completion/next will start playing the next track in playlist
        if( command == COMMAND_NONE || command == COMMAND_NEXT )
        {
            if( pContext->playlist.position + 1 < pContext->playlist.size )
            {
                ML_LOG(Info, "%s() line: %d a track - which is not the last one - has completed playing", __FUNCTION__, __LINE__);
                // a track - which is not the last one - has completed playing
                // continue with the next track in playlist
                int32_t position = pContext->playlist.position + 1;

                ML_LOG(Info, "%s() line: %d continue with the next track: %d in playlist: \"%s\"",
                        __FUNCTION__, __LINE__, position, pContext->playlist.uris[position]);

                PlaylistPlayIndex(pContext, position, true);
            }
            else
            {
                ML_LOG(Info, "%s() line: %d last track in playlist has completed playing", __FUNCTION__, __LINE__);
                // last track in playlist has completed playing
                // if the current track is the last one in playlist, behavior is determined by repeat_album
                if( pContext->repeatState == MLMusicServiceRepeatState_Album)
                {
                    // start from the beginning if repeat_album is set
                    int32_t position = 0;

                    ML_LOG(Info, "%s() line: %d continue with the first track: %d in playlist: \"%s\"",
                            __FUNCTION__, __LINE__, position, pContext->playlist.uris[position]);

                    PlaylistPlayIndex(pContext, position, true);
                }
                else
                {
                    // if current track is the last one and repeat is off;
                    if( command == COMMAND_NONE ) {
                        // move to a stopped state such that the next play command will play the first track in playlist
                        // in other words, set the next track but don't start playing

                        PlaylistPlayIndex(pContext, 0, false);
                    }
                    else {
                        // if next button is pressed; do nothing, keep playing current track
                    }
                }
            }
        }
        else if( command == COMMAND_PREVIOUS)
        {
            // if current track is not the first one in playlist; pressing previous button will start playing the previous track in playlist
            if( pContext->playlist.position - 1 >= 0 )
            {

                int32_t position = pContext->playlist.position - 1;

                ML_LOG(Info, "%s() line: %d continue with the previous track: %d in playlist: \"%s\"",
                        __FUNCTION__, __LINE__, position, pContext->playlist.uris[position]);

                PlaylistPlayIndex(pContext, position, true);
            }
            else
            {
                // if the current track is the first one in playlist, previous button will restart the current track
                // but it will never move to last track regardless of repeat_album
                int32_t position = 0;

                ML_LOG(Info, "%s() line: %d pressing previous while playing the first track will restart it: %d in playlist: \"%s\"",
                        __FUNCTION__, __LINE__, position, pContext->playlist.uris[position]);

                PlaylistPlayIndex(pContext, position, true);
            }
        }
    }

    return MLResult_Ok;
}

// report service specific errors to application as is
// MLMusicServiceProviderNotifyError( MLMusicServiceError_ServiceSpecific, (int32_t)error );


// note: must run on a separate thread because main() is blocked at MLMusicServiceProviderStart()
// note: do not exit this thread, then it will result in zombie process because main() will block indefinitely

// all these commands run in the same thread ie; ThreadMain handles decode/demux/render operations

void* ThreadMain(Context* pContext)
{
    ML_LOG(Info, "%s() line: %d enter", __FUNCTION__, __LINE__);

    if( NULL == pContext )
    {
        ML_LOG(Error, "example_music_provider.ThreadMain failed. Reason: pContext was null.");
        return NULL;
    }

    MLResult result = MLResult_Ok;
    for(;;) {
        // process command if exists

        switch (pContext->command) {

        case COMMAND_OPEN:
            ML_LOG(Info, "%s() line: %d COMMAND_OPEN desiredUri: \"%s\"", __FUNCTION__, __LINE__, pContext->desiredUri);

            result = AudioOpen( &pContext->pDecoderContext,  pContext->desiredUri, "audio/");
            if( result == MLResult_Ok && pContext->pDecoderContext != NULL)
            {
                strncpy( pContext->mediaUri, pContext->desiredUri, sizeof(pContext->mediaUri) );

                MLMusicServiceProviderNotifyMetadataChange();

                pContext->playbackState = MLMusicServicePlaybackState_Playing;
                MLMusicServiceProviderNotifyPlaybackStateChange(pContext->playbackState);
            }

            pContext->command = COMMAND_NONE;
            break;

        // TODO: rename this as play button
        case COMMAND_START:
            ML_LOG(Info, "%s() line: %d COMMAND_START", __FUNCTION__, __LINE__);

            if( pContext->playbackState == MLMusicServicePlaybackState_Paused )
            {
                // delegate to resume
                ML_LOG(Info, "%s() line: %d COMMAND_START STATE_PAUSED delegate to COMMAND_RESUME", __FUNCTION__, __LINE__);
                pContext->command = COMMAND_RESUME;
                continue;
            }
            else if( pContext->playbackState == MLMusicServicePlaybackState_Stopped )
            {
                if( strlen(pContext->mediaUri) > 0 )
                {
                    // media uri never opened or alternatively stopped
                    if( pContext->pDecoderContext == NULL)
                    {
                        result = AudioOpen( &pContext->pDecoderContext, pContext->mediaUri, "audio/");
                        if( result == MLResult_Ok && pContext->pDecoderContext != NULL)
                        {
                            pContext->playbackState = MLMusicServicePlaybackState_Playing;
                            MLMusicServiceProviderNotifyPlaybackStateChange(pContext->playbackState);
                        }
                    }
                }
                else
                {
                    ML_LOG(Error, "%s() line: %d COMMAND_START error! no media uri specified", __FUNCTION__, __LINE__);
                }
            }

            pContext->command = COMMAND_NONE;
            break;

        case COMMAND_PAUSE:
            ML_LOG(Info, "%s() line: %d COMMAND_PAUSE", __FUNCTION__, __LINE__);

            // perform pause by not calling demux and decode, codec does not know being in paused mode - no need to flush codec buffers
            // trying to pause by calling MediaCodecStop(), then MediaCodecStart() does not work
            // do not flush codec here, because we want to use the samples and frames currently in media pipeline when resumed

            MLMusicServiceProviderFlushAudioOutput();

            pContext->playbackState = MLMusicServicePlaybackState_Paused;
            MLMusicServiceProviderNotifyPlaybackStateChange(pContext->playbackState);

            pContext->command = COMMAND_NONE;
            break;

        case COMMAND_RESUME:
            ML_LOG(Info, "%s() line: %d COMMAND_RESUME", __FUNCTION__, __LINE__);

            // you can only resume in paused state, not while playing or stopped states
            if( pContext->playbackState == MLMusicServicePlaybackState_Paused)
            {
                pContext->playbackState = MLMusicServicePlaybackState_Playing;
                MLMusicServiceProviderNotifyPlaybackStateChange(pContext->playbackState);
            }

            pContext->command = COMMAND_NONE;
            break;

        case COMMAND_SEEK:
            ML_LOG(Info, "%s() line: %d COMMAND_SEEK", __FUNCTION__, __LINE__);

            MLMusicServiceProviderFlushAudioOutput();
            result = AudioSeek( pContext->pDecoderContext, (int64_t) pContext->desiredSeekPosition * 1000);

            ML_LOG(Info, "%s() line: %d COMMAND_SEEK AudioSeek() returned: %d", __FUNCTION__, __LINE__, (int)result);

            pContext->command = COMMAND_NONE;
            break;

        case COMMAND_STOP:
            ML_LOG(Info, "%s() line: %d COMMAND_STOP", __FUNCTION__, __LINE__);

            MLMusicServiceProviderFlushAudioOutput();

            if( NULL != pContext->pDecoderContext)
            {
                AudioRelease( pContext->pDecoderContext );
                pContext->pDecoderContext = NULL;
            }

            pContext->playbackState = MLMusicServicePlaybackState_Stopped;
            MLMusicServiceProviderNotifyPlaybackStateChange(pContext->playbackState);

            pContext->command = COMMAND_NONE;
            break;

        case COMMAND_NEXT:
            ML_LOG(Info, "%s() line: %d COMMAND_NEXT", __FUNCTION__, __LINE__);

            MLMusicServiceProviderFlushAudioOutput();
            MLMusicServiceProviderNotifyStatus(MLMusicServiceStatus_NextTrack);

            pContext->command = COMMAND_NONE;
            HandlePlaylist( pContext, COMMAND_NEXT);
            break;

        case COMMAND_PREVIOUS:
            ML_LOG(Info, "%s() line: %d COMMAND_PREVIOUS", __FUNCTION__, __LINE__);

            MLMusicServiceProviderFlushAudioOutput();
            MLMusicServiceProviderNotifyStatus(MLMusicServiceStatus_PrevTrack);

            pContext->command = COMMAND_NONE;
            HandlePlaylist( pContext, COMMAND_PREVIOUS);
            break;

        case COMMAND_EXIT:
            // user or the system signaled exit
            ML_LOG(Info, "%s() line: %d COMMAND_EXIT", __FUNCTION__, __LINE__);

            MLMusicServiceProviderFlushAudioOutput();

            if( pContext->playbackState != MLMusicServicePlaybackState_Stopped)
            {
                pContext->playbackState = MLMusicServicePlaybackState_Stopped;
                MLMusicServiceProviderNotifyPlaybackStateChange(pContext->playbackState);
            }
            pContext->command = COMMAND_NONE;
            return pContext;
            break;
        case COMMAND_NONE:
        default:
            break;
        }

        if( MLMusicServicePlaybackState_Playing == pContext->playbackState )
        {
            // note: make these 2 functions return the number of demuxed samples and decoded frames
            //       then you can adjust your waiting in accordance with them
            int32_t samples_demuxed = 0; // in this call only
            result = AudioProcessInput (
                            pContext->pDecoderContext,
                            &samples_demuxed);

            int32_t frames_decoded = 0; // in this call only
            result = AudioProcessOutput(
                            pContext->pDecoderContext,
                            &frames_decoded,
                            pContext,
                            OnAudioFormatChanged,
                            OnAudioFrameDecoded );

            bool decode_completed = false;
            result = AudioGetDecodeCompleted( pContext->pDecoderContext, &decode_completed);

            if( MLResult_Ok == result && decode_completed )
            {
                HandlePlaylist(pContext, COMMAND_NONE);
            }

            // demuxer and decoder works without timeouts, hence we have to sleep to avoid infinite loop
            // because we do not wait at all for demuxing/decoding we can add a meager one
            // do not wait at all when you just grabbed a decoded frame, because there is now an input slot for sure
            if ( samples_demuxed == 0 && frames_decoded  == 0)
            {
                //    //ML_LOG(Info, "%s() line: %d command: %d state: %d sleeping: %d...",
                //    //        __FUNCTION__, __LINE__, pContext->command, pContext->state, POLLING_PERIOD*10 );
                usleep(1000);
            }
        }
        else
        {
            // no command to process and no demux/decode is going on
            // the sleep length here only determines reaction time to a user command on terminal
            usleep(1000);
            //ML_LOG(Info, "%s() line: %d command: %d state: %d sleeping: %d...",
            //        __FUNCTION__, __LINE__, pContext->command, pContext->state, POLLING_PERIOD*10 );
        }
    }

    if( NULL != pContext )
    {
        if( NULL != pContext->pDecoderContext)
        {
            AudioRelease( pContext->pDecoderContext );
            pContext->pDecoderContext = NULL;
        }

        if( pContext->playlist.uris != NULL)
        {
            // release each uri
            for(int i=0; i < pContext->playlist.size; i++)
            {
                free(pContext->playlist.uris[i]);
                pContext->playlist.uris[i] = NULL;
            }

            // release playlist
            free(pContext->playlist.uris);
            pContext->playlist.uris = NULL;
        }

    }

    // thread returns context pointer at success, NULL at failure
    // - when context pointer is received as NULL, we have no other pointer value to return
    ML_LOG(Info, "%s() line: %d exit return: %d", __FUNCTION__, __LINE__, result);
    return MLResult_Ok == result ? pContext : NULL;
}

int main(int argc __unused, char* argv[] __unused)
{
    ML_LOG(Info, "%s() line: %d enter", __FUNCTION__, __LINE__);

    Playlist playlist =
    {
        NULL,   // uri array
           0,   // size
          -1,   // position
    };

    Context context =
    {
        COMMAND_NONE,                        // command
        MLMusicServicePlaybackState_Stopped, // playbackState
        NULL,                                // decoder context
        "",                                  // mediaUri
        MLMusicServiceRepeatState_Off,       // repeatState
        MLMusicServiceShuffleState_Off,      // shuffleState
        "",                                  // desiredUri
        0,                                   // desiredSeekPosition
        playlist,                            // playlist
    };

    MLMusicServiceProviderImplementation musicServiceImpl =
    {
        &BMSProviderSetAuthString,
        &BMSProviderSetUrl,          // N/A
        &BMSProviderSetPlaylist,
        &BMSProviderStart,
        &BMSProviderStop,
        &BMSProviderPause,
        &BMSProviderResume,
        &BMSProviderSeek,
        &BMSProviderNext,
        &BMSProviderPrevious,
        &BMSProviderSetShuffle,
        &BMSProviderSetRepeat,
        &BMSProviderSetVolume,
        &BMSProviderGetTrackLength,
        &BMSProviderGetCurrentPosition,
        &BMSProviderGetMetadata,
        &BMSProviderOnEndService,
        &context
    };

    MLResult result = MLResult_Ok;
    ML_LOG(Info, "%s() line: %d MLMusicServiceProviderCreate", __FUNCTION__, __LINE__);

    result = MLMusicServiceProviderCreate(&musicServiceImpl);
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.main call to MLMusicServiceProviderCreate failed. Reason: %s.", MLMediaResultGetString(result));
        return result;
    }

    pthread_t thread;
    // Have the playback run in the background
    if( pthread_create(&thread, NULL, (void*) ThreadMain, &context) != 0 )
    {
        ML_LOG(Error, "example_music_provider.main failed. Reason: failed to create thread\n");
        ML_LOG(Info, "%s() line: %d exit return: %s", __FUNCTION__, __LINE__, MLMediaResultGetString(MLResult_UnspecifiedFailure));
        return EXIT_FAILURE;
    }

    // note: this call blocks! this is why the thread is launched beforehand
    // note: this should match the visible_name field of provider component in manifest.xml
    // note: this call never returns even after BMSProviderOnEndService callback.
    result = MLMusicServiceProviderStart("ExampleMusicProvider");
    ML_LOG(Info, "%s() line: %d MLMusicServiceProviderStart returned: %s", __FUNCTION__, __LINE__, MLMediaResultGetString(result));
    if (MLResult_Ok != result)
    {
        ML_LOG(Error, "example_music_provider.main call to MLMusicServiceProviderStart failed. Reason: %s", MLMediaResultGetString(result));
        return EXIT_FAILURE;
    }

    // thread returns context pointer at success, NULL at failure
    void* result_thread = NULL;
    if( pthread_join(thread, &result_thread) != 0 || result_thread == NULL)
    {
        ML_LOG(Error, "example_music_provider.main failed. Reason: failed to join thread\n");
        ML_LOG(Info, "%s() line: %d exit return: %s", __FUNCTION__, __LINE__, MLMediaResultGetString(MLResult_UnspecifiedFailure));
        return EXIT_FAILURE;
    }

    ML_LOG(Info, "%s() line: %d exit return: %s", __FUNCTION__, __LINE__, MLMediaResultGetString(result));
    return EXIT_SUCCESS;
}
