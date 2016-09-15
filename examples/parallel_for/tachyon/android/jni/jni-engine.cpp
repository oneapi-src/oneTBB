/*
    Copyright 2005-2016 Intel Corporation.  All Rights Reserved.

    The source code contained or described herein and all documents related
    to the source code ("Material") are owned by Intel Corporation or its
    suppliers or licensors.  Title to the Material remains with Intel
    Corporation or its suppliers and licensors.  The Material is protected
    by worldwide copyright laws and treaty provisions.  No part of the
    Material may be used, copied, reproduced, modified, published, uploaded,
    posted, transmitted, distributed, or disclosed in any way without
    Intel's prior express written permission.

    No license under any patent, copyright, trade secret or other
    intellectual property right is granted to or conferred upon you by
    disclosure or delivery of the Materials, either expressly, by
    implication, inducement, estoppel or otherwise.  Any license under such
    intellectual property rights must be express and approved by Intel in
    writing.
*/

/*
    The original source for this example is
    Copyright (c) 1994-2008 John E. Stone
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    3. The name of the author may not be used to endorse or promote products
       derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
    OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
 */

#include <jni.h>
#include <android/bitmap.h>
#include <pthread.h>
#include <signal.h>

#include "utility/utility.h"
#include "tachyon_video.h"
#include "tbb/tick_count.h"
#include "tbb/task_scheduler_init.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <android/log.h>
#define  LOG_INFO(...)  __android_log_print(ANDROID_LOG_INFO,"tachyon",__VA_ARGS__)
#define  LOG_ERROR(...)  __android_log_print(ANDROID_LOG_ERROR,"tachyon",__VA_ARGS__)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "api.h"       /* The ray tracing library API */
#include "parse.h"     /* Support for my own file format */
#include "ui.h"
#include "util.h"

SceneHandle global_scene;
int global_xsize;     /*  size of graphic image rendered in window (from hres, vres)  */
int global_ysize;
int global_xwinsize;  /*  size of window (may be larger than above)  */
int global_ywinsize;
bool global_usegraphics;
char* global_window_title;
static long startTime=0;
static volatile long elapsedTime=0;
static volatile bool isCancelled=false;
static volatile bool isPaused=false;

bool silent_mode = false; /* silent mode */

class tachyon_video *video = 0;

typedef struct {
    int foundfilename;      /* was a model file name found in the args? */
    char filename[1024];    /* model file to render */
    int useoutfilename;     /* command line override of output filename */
    char outfilename[1024]; /* name of output image file */
    int verbosemode;        /* verbose flags */
    int antialiasing;       /* antialiasing setting */
    int displaymode;        /* display mode */
    int boundmode;          /* bounding mode */
    int boundthresh;        /* bounding threshold */
    int usecamfile;         /* use camera file */
    char camfilename[1024]; /* camera filename */
} argoptions;

void initoptions(argoptions * opt) {
    memset(opt, 0, sizeof(argoptions));
    opt->foundfilename = -1;
    opt->useoutfilename = -1;
    opt->verbosemode = -1;
    opt->antialiasing = -1;
    opt->displaymode = -1;
    opt->boundmode = -1;
    opt->boundthresh = -1;
    opt->usecamfile = -1;
}

int CreateScene(argoptions &opt) {
    char *filename;

    global_scene = rt_newscene();
    rt_initialize();

    //filename = "/mnt/sdcard/tachyon/data.dat";
    filename = opt.filename;
    LOG_INFO("CreateScene: data file name is %s", filename);

    LOG_INFO("Readmodel");
    if (readmodel(filename, global_scene) != 0) {
        LOG_ERROR("Parser returned a non-zero error code reading %s\n", filename);
        LOG_ERROR("Aborting Render...\n");
        rt_finalize();
        return -1;
    }
    LOG_INFO("Readmodel done");

    scenedef *scene = (scenedef *) global_scene;

    //scene->hres and scene->yres are taken from display properties in *initBitmap() function
    scene->hres = global_xwinsize = global_xsize;
    scene->vres = global_ywinsize = global_ysize;
    LOG_INFO("CreateScene: global_xsize=%d global_ysize=%d", global_xsize, global_ysize);

    return 0;
}

extern unsigned int * g_pImg;

void* example_main(void *filename) {
    try {
        LOG_INFO("initoptions");
        argoptions opt;
        initoptions(&opt);
        strcpy(opt.filename, (char*) filename);
        LOG_INFO("initoptions done");

        LOG_INFO("CreateScene");
        if ((CreateScene(opt) != 0))
            return NULL;
        LOG_INFO("CreateScene done");

        LOG_INFO("tachyon video");
        tachyon_video tachyon;
        LOG_INFO("tachyon video init_console");
        tachyon.init_console();
        LOG_INFO("tachyon video init_window");

        tachyon.init_window(global_xsize, global_ysize);
        LOG_INFO("tachyon video init_window done");
        if(  !tachyon.running )
            return NULL;
        LOG_INFO("tachyon video done");

        video = &tachyon;
        //Exit from the while via GUI "Exit" menu.
        for(;;) {
            LOG_INFO("main_loop() start");
            elapsedTime = 0;
            startTime=time(NULL);
            isCancelled=false;
            if (video)video->running = true;
            memset(g_pImg, 0, 4 * global_xsize * global_ysize);
            tachyon.main_loop();
            elapsedTime = (long)(time(NULL)-startTime);
            video->running=false;
            //The timer to restart drawing then it is complete.
            int timer=5;
            do{
                sleep(1);
            }while( (  isPaused || !isCancelled && (timer--)>0 ) );
            LOG_INFO("main_loop() done");
        }
        return NULL;
    } catch (std::exception& e) {
        LOG_ERROR("An error occurred. Error text is %s", e.what());
        return NULL;
    }
}

static int fill_rect(void* pixels, int size) {
    if( pixels && g_pImg ){
        memcpy(pixels, g_pImg, size);
        if (video->running)
            elapsedTime=(long)(time(NULL)-startTime);
        return 0;
    }else{
        return -1;
    }
}

extern "C" JNIEXPORT jlong JNICALL Java_com_intel_tbb_example_tachyon_tachyonView_getElapsedTime(
        JNIEnv * env) {
    return  elapsedTime;
}

extern "C" JNIEXPORT void JNICALL Java_com_intel_tbb_example_tachyon_tachyonView_initBitmap(
        JNIEnv * env, jobject obj, jobject bitmap, jint x_size, jint y_size,
        jint number_of_threads, jstring filename) {
    LOG_INFO("initBitmap start");
    static pthread_t handle;
    char buf[5];
    LOG_INFO("video");
    //TBB_NUM_THREADS is reading somewhere inside C++ common code
    if (number_of_threads >= 0 && number_of_threads < 256) {
        snprintf(buf, 4, "%d", number_of_threads);
        setenv("TBB_NUM_THREADS", buf, 1);
    }
    LOG_INFO("TBB_NUM_THREADS=%s",getenv ("TBB_NUM_THREADS"));
    //Cancel if we are in the middle of the painting
    isCancelled = true;
    if (video)
        video->running = false;
    if (!handle) {
        LOG_INFO("Starting native thread");
        pthread_attr_t s;
        pthread_attr_init(&s);
        //adjusting picture to physical resolution
        global_xsize = x_size;
        global_ysize = y_size;
        char const* fn = env->GetStringUTFChars(filename, NULL);
        LOG_INFO("fn=%s",fn);
        //Starting example_main and returning back to GUI
        pthread_create(&handle, NULL, &example_main, (void*) fn);
        LOG_INFO("Thread handle is %ld", handle);
    }
}

extern "C" JNIEXPORT jint JNICALL Java_com_intel_tbb_example_tachyon_tachyonView_renderBitmap(
        JNIEnv * env, jobject obj, jobject bitmap, jint size) {
    AndroidBitmapInfo info;
    void* pixels;
    int ret;

    //Getting bitmap to fill
    if ((ret = AndroidBitmap_getInfo(env, bitmap, &info)) < 0) {
        LOG_ERROR("AndroidBitmap_getInfo() returned %d", ret);
        return ret;
    }

    //Locking bitmap to fill
    if ((ret = AndroidBitmap_lockPixels(env, bitmap, &pixels)) < 0) {
        LOG_ERROR("AndroidBitmap_lockPixels() returned %d", ret);
    }

    //Filling the bitmap
    ret = fill_rect(pixels, size);

    //Unlocking the bitmap
    AndroidBitmap_unlockPixels(env, bitmap);
    return ret;
}

extern "C" JNIEXPORT void JNICALL Java_com_intel_tbb_example_tachyon_tachyon_setPaused(
        JNIEnv * env, jobject obj, jboolean paused) {
    if(video)video->pausing = isPaused = paused;
    return;
}
