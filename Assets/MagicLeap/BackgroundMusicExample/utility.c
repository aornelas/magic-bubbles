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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include <ml_media_format.h>
#include <ml_media_error.h>
#include <ml_audio.h>
#include <ml_lifecycle.h>
#include <ml_logging.h>
#include <ml_music_service_provider.h>

#include "utility.h"

// try
//      <path>                                                     "/data/local/tmp/abc.mp3"
//      <self_info->package_dir_path>/resources/<path>:            "/data/app/com.magicleap.capi.test.bms_demo_player/resources/CleopatraStratan_ZuneaZunea.mp3""
//      <self_info->writable_dir_path_locked_and_unlocked>/<path>: "/documents/C1/"
//      <self_info->writable_dir_path>/<path>:                     "/documents/C2/"

// "ShortAudio2.mp3" ==> "/package/resources/ShortAudio2.mp3"
// "/package/resources/ShortAudio2.mp3" ==> "/package/resources/ShortAudio2.mp3"
// mldb push -p com.magicleap.capi.test.bms_demo_player ~/Videos/echo-hereweare.mp4 /documents/C1/echo-hereweare_1.mp4
// "echo-hereweare_1.mp4" ==> "/documents/C2/echo-hereweare_1.mp4"
// mldb push -p com.magicleap.capi.test.bms_demo_player ~/Videos/echo-hereweare.mp4 /documents/C2/echo-hereweare_2.mp4
// "echo-hereweare_2.mp4" ==> "/documents/C2/echo-hereweare_2.mp4"
// "/documents/C2/echo-hereweare_2.mp4" ==> "/documents/C2/echo-hereweare_2.mp4"
// but the package does not have access to everything
// "/data/local/tmp/abc.mp3" ==> not found because package has no access to the folder

MLResult SearchMedia(const char* uri, char* media_path, size_t media_path_max)
{
    MLLifecycleSelfInfo* self_info = NULL;
    struct stat stat_info = {};
    MLResult result = MLResult_Ok;

    if( NULL == uri )
    {
        ML_LOG(Error, "example_music_provider.SearchMedia failed. Reason: uri is null.");
        return MLResult_InvalidParam;
    }

    if( NULL == media_path )
    {
        ML_LOG(Error, "example_music_provider.SearchMedia failed. Reason: media_path is null.");
        return MLResult_InvalidParam;
    }

    if( !(media_path_max > 0) )
    {
        ML_LOG(Error, "example_music_provider.SearchMedia failed. Reason: invalid media path max.");
        return MLResult_InvalidParam;
    }

    // local media filepaths may optionally be prepended "file://"; just skip
    if( strncmp(uri, "file://", sizeof("file://")-1) == 0) {
        uri += sizeof("file://")-1;
    }

    if( strncmp(uri, "http://",  sizeof("http://") -1) == 0 ||
        strncmp(uri, "https://", sizeof("https://")-1) == 0 ||
        strncmp(uri, "rtsp://",  sizeof("rtsp://") -1) == 0)
    {
        // copy urls as is
        strncpy( media_path, uri, media_path_max);
        if( NULL != self_info)
        {
            MLLifecycleFreeSelfInfo(&self_info);
            self_info = NULL;
        }

        return result;
    }

    //
    // try absolute path
    //      eg; "/documents/C2/echo-hereweare_2.mp4"

    strncpy( media_path, uri, media_path_max);

    memset(&stat_info, 0, sizeof(struct stat));
    if( stat(media_path, &stat_info) == 0 ||
        S_ISREG(stat_info.st_mode) )  {
        ML_LOG(Info, "%s() line: %d uri: \"%s\" found: \"%s\"", __FUNCTION__, __LINE__, uri, media_path);
        if( NULL != self_info)
        {
            MLLifecycleFreeSelfInfo(&self_info);
            self_info = NULL;
        }

        return result;
    }
    else {
        ML_LOG(Info, "%s() line: %d uri: \"%s\" not found: \"%s\"", __FUNCTION__, __LINE__, uri, media_path);
    }

    result = MLLifecycleGetSelfInfo(&self_info);
    if( MLResult_Ok != result )
    {
        ML_LOG(Error, "example_music_provider.SearchMedia call to MLLifecycleGetSelfInfo failed. Reason: %s.", MLMediaResultGetString(result));
        return result;
    }

    if( NULL == self_info )
    {
        ML_LOG(Error, "example_music_provider.SearchMedia failed. Reason: self_info is null.");
        return MLResult_UnspecifiedFailure;
    }

    //writable_dir_path: "/data/user/1/apps/com.magicleap.capi.test.bms_demo_player/documents/C2/"
    //package_dir_path : "/data/app/com.magicleap.capi.test.bms_demo_player"
    //package_name     : "com.magicleap.capi.test.bms_demo_player"
    //component_name   : "BMSDemoService"
    //tmp_dir_path     : "/data/user/1/apps/com.magicleap.capi.test.bms_demo_player/tmp/"
    //visible_name     : "bms_demo_player"
    //writable_dir_path_locked_and_unlocked : "/data/user/1/apps/com.magicleap.capi.test

    /*
    //Path to the writable dir of the application. valid when the device is unlocked - not locked
    ML_LOG(Info, "%s() line: %d writable_dir_path: \"%s\" ", __FUNCTION__, __LINE__,
            self_info->writable_dir_path);
    // Path to the application package dir
    ML_LOG(Info, "%s() line: %d package_dir_path : \"%s\" ", __FUNCTION__, __LINE__,
            self_info->package_dir_path);
    // Package name of the application.
    ML_LOG(Info, "%s() line: %d package_name     : \"%s\" ", __FUNCTION__, __LINE__,
            self_info->package_name);
    ML_LOG(Info, "%s() line: %d component_name   : \"%s\" ", __FUNCTION__, __LINE__,
            self_info->component_name);
    // Path to the application tmp dir.
    ML_LOG(Info, "%s() line: %d tmp_dir_path     : \"%s\" ", __FUNCTION__, __LINE__,
            self_info->tmp_dir_path);
    // Visible name of the application
    ML_LOG(Info, "%s() line: %d visible_name     : \"%s\" ", __FUNCTION__, __LINE__,
            self_info->visible_name);
    // this path is always available to an application - locked or unlocked .
    ML_LOG(Info, "%s() line: %d writable_dir_path_locked_and_unlocked : \"%s\" ", __FUNCTION__, __LINE__,
            self_info->writable_dir_path_locked_and_unlocked);
    */
    //
    // try  <self_info->package_dir_path>/resources/<path
    //

    strncpy( media_path, self_info->package_dir_path, media_path_max);
    if( self_info->package_dir_path[strlen(self_info->package_dir_path)-1] != '/') {
        strncat( media_path, "/", media_path_max);
    }
    strncat( media_path, "resources/", media_path_max);
    strncat( media_path, uri, media_path_max);

    memset(&stat_info, 0, sizeof(struct stat));
    if( stat(media_path, &stat_info) == 0 ||
        S_ISREG(stat_info.st_mode) )  {
        ML_LOG(Info, "%s() line: %d uri: \"%s\" found: \"%s\"", __FUNCTION__, __LINE__, uri, media_path);
        if( NULL != self_info)
        {
            MLLifecycleFreeSelfInfo(&self_info);
            self_info = NULL;
        }

        return result;
    }
    else {
        ML_LOG(Info, "%s() line: %d uri: \"%s\" not found: \"%s\"", __FUNCTION__, __LINE__, uri, media_path);
    }

    //
    // try <self_info->writable_dir_path_locked_and_unlocked>/<path>
    //

    strncpy( media_path, self_info->writable_dir_path_locked_and_unlocked, media_path_max);
    if( self_info->writable_dir_path_locked_and_unlocked[strlen(self_info->writable_dir_path_locked_and_unlocked)-1] != '/') {
        strncat( media_path, "/", media_path_max);
    }
    strncat( media_path, uri, media_path_max);

    memset(&stat_info, 0, sizeof(struct stat));
    if( stat(media_path, &stat_info) == 0 ||
        S_ISREG(stat_info.st_mode) )  {
        ML_LOG(Info, "%s() line: %d uri: \"%s\" found: \"%s\"", __FUNCTION__, __LINE__, uri, media_path);
        if( NULL != self_info)
        {
            MLLifecycleFreeSelfInfo(&self_info);
            self_info = NULL;
        }

        return result;
    }
    else {
        ML_LOG(Info, "%s() line: %d uri: \"%s\" not found: \"%s\"", __FUNCTION__, __LINE__, uri, media_path);
    }

    //
    // try <self_info->writable_dir_path>/<path> ie;
    //

    strncpy( media_path, self_info->writable_dir_path, media_path_max);
    if( self_info->writable_dir_path[strlen(self_info->writable_dir_path)-1] != '/') {
        strncat( media_path, "/", media_path_max);
    }
    strncat( media_path, uri, media_path_max);

    memset(&stat_info, 0, sizeof(struct stat));
    if( stat(media_path, &stat_info) == 0 ||
        S_ISREG(stat_info.st_mode) )  {
        ML_LOG(Info, "%s() line: %d file: \"%s\" found: \"%s\"", __FUNCTION__, __LINE__, uri, media_path);
        if( NULL != self_info)
        {
            MLLifecycleFreeSelfInfo(&self_info);
            self_info = NULL;
        }

        return result;
    }
    else {
        ML_LOG(Info, "%s() line: %d file: \"%s\" not found: \"%s\"", __FUNCTION__, __LINE__, uri, media_path);
    }

    // file not found in folders searched
    result = MLResult_UnspecifiedFailure;

    if( NULL != self_info)
    {
        MLLifecycleFreeSelfInfo(&self_info);
        self_info = NULL;
    }

    return result;
}
