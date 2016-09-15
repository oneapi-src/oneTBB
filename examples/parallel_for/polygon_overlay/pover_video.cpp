/*
    Copyright 2005-2016 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks. Threading Building Blocks is free software;
    you can redistribute it and/or modify it under the terms of the GNU General Public License
    version 2  as  published  by  the  Free Software Foundation.  Threading Building Blocks is
    distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See  the GNU General Public License for more details.   You should have received a copy of
    the  GNU General Public License along with Threading Building Blocks; if not, write to the
    Free Software Foundation, Inc.,  51 Franklin St,  Fifth Floor,  Boston,  MA 02110-1301 USA

    As a special exception,  you may use this file  as part of a free software library without
    restriction.  Specifically,  if other files instantiate templates  or use macros or inline
    functions from this file, or you compile this file and link it with other files to produce
    an executable,  this file does not by itself cause the resulting executable to be covered
    by the GNU General Public License. This exception does not however invalidate any other
    reasons why the executable file might be covered by the GNU General Public License.
*/

// Support for GUI display for Polygon overlay demo

#define VIDEO_WINMAIN_ARGS
#include <iostream>
#include "polyover.h"
#include "polymain.h"
#include "pover_video.h"
#include "tbb/tick_count.h"
#include "tbb/task_scheduler_init.h"
#ifndef _WIN32
#include <sys/time.h>
#include <unistd.h>

void rt_sleep(int msec) {
    usleep(msec*1000);
}

#else //_WIN32

#undef OLDUNIXTIME
#undef STDTIME

#include <windows.h>

void rt_sleep(int msec) {
    Sleep(msec);
}

#endif  /*  _WIN32  */

using namespace std;

bool g_next_frame() {
    if(++n_next_frame_calls >= frame_skips) { // the data race here is benign
        n_next_frame_calls = 0; 
        return gVideo->next_frame();
    } 
    return gVideo->running;
}

bool g_last_frame() {
    if(n_next_frame_calls) return gVideo->next_frame(); 
    return gVideo->running;
}

bool initializeVideo(int argc, char **argv) {
    //pover_video *l_video = new pover_video();
    //gVideo = l_video;
    gVideo->init_console();  // don't check return code.
    gVideo->title = g_windowTitle;
    g_useGraphics = gVideo->init_window(g_xwinsize, g_ywinsize);
    return true;
}

void pover_video::on_process() {
    tbb::tick_count t0, t1;
    double naiveParallelTime, domainSplitParallelTime;
    // create map1  These could be done in parallel, if the pseudorandom number generator were re-seeded.
    GenerateMap(&gPolymap1, gMapXSize, gMapYSize, gNPolygons, /*red*/255, /*green*/0, /*blue*/127);
    // create map2
    GenerateMap(&gPolymap2, gMapXSize, gMapYSize, gNPolygons, /*red*/0, /*green*/255, /*blue*/127);
        //
        // Draw source maps
    gDrawXOffset = map1XLoc;
    gDrawYOffset = map1YLoc;
        for(int i=0; i < int(gPolymap1->size()); i++) {
            (*gPolymap1)[i].drawPoly();
        }
    gDrawXOffset = map2XLoc;
    gDrawYOffset = map2YLoc;
        for(int i=0; i < int(gPolymap2->size()) ;i++) {
            (*gPolymap2)[i].drawPoly();
        }
        gDoDraw = true;

    // run serial map generation
    gDrawXOffset = maprXLoc;
    gDrawYOffset = maprYLoc;
    {
        RPolygon *xp = new RPolygon(0, 0, gMapXSize-1, gMapYSize-1, 0, 0, 0);  // Clear the output space
        delete xp;
        t0 = tbb::tick_count::now();
        SerialOverlayMaps(&gResultMap, gPolymap1, gPolymap2);
        t1 = tbb::tick_count::now();
        cout << "Serial overlay took " << (t1-t0).seconds()*1000 << " msec" << std::endl;
        gSerialTime = (t1-t0).seconds()*1000;
#if _DEBUG
        CheckPolygonMap(gResultMap);
        // keep the map for comparison purposes.
#else
        delete gResultMap;
#endif
        if(gCsvFile.is_open()) {
            gCsvFile << "Serial Time," << gSerialTime << std::endl;
            gCsvFile << "Threads,";
            if(gThreadsLow == THREADS_UNSET || gThreadsLow == tbb::task_scheduler_init::automatic) {
                gCsvFile << "Threads,Automatic";
            }
            else {
                for(int i=gThreadsLow; i <= gThreadsHigh; i++) {
                    gCsvFile << i;
                    if(i < gThreadsHigh) gCsvFile << ",";
                }
            }
            gCsvFile << std::endl;
        }
        if(gIsGraphicalVersion) rt_sleep(2000);
    }
    // run naive parallel map generation
    {
        Polygon_map_t *resultMap;
        if(gCsvFile.is_open()) {
            gCsvFile << "Naive Time";
        }
        NaiveParallelOverlay(resultMap, *gPolymap1, *gPolymap2);
        delete resultMap;
        if(gIsGraphicalVersion) rt_sleep(2000);
    }
    // run split map generation
    {
        Polygon_map_t *resultMap;
        if(gCsvFile.is_open()) {
            gCsvFile << "Split Time";
        }
        SplitParallelOverlay(&resultMap, gPolymap1, gPolymap2);
        delete resultMap;
        if(gIsGraphicalVersion) rt_sleep(2000);
    }
    // split, accumulating into concurrent vector
    {
        concurrent_Polygon_map_t *cresultMap;
        if(gCsvFile.is_open()) {
            gCsvFile << "Split CV time";
        }
        SplitParallelOverlayCV(&cresultMap, gPolymap1, gPolymap2);
        delete cresultMap;
        if(gIsGraphicalVersion) rt_sleep(2000);
    }
    // split, accumulating into ETS
    {
        ETS_Polygon_map_t *cresultMap;
        if(gCsvFile.is_open()) {
            gCsvFile << "Split ETS time";
        }
        SplitParallelOverlayETS(&cresultMap, gPolymap1, gPolymap2);
        delete cresultMap;
        if(gIsGraphicalVersion) rt_sleep(2000);
    }
    if(gIsGraphicalVersion) rt_sleep(8000);
    delete gPolymap1;
    delete gPolymap2;
#if _DEBUG
    delete gResultMap;
#endif
}
