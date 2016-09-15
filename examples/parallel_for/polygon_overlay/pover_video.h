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

// support for GUI for polygon overlay demo
//
#ifndef _POVER_VIDEO_H_
#define _POVER_VIDEO_H_
#include "../../common/gui/video.h"

#include "pover_global.h"  // for declaration of DEFINE and INIT

DEFINE class video *gVideo INIT(0);

DEFINE int n_next_frame_calls INIT(0);
DEFINE int frame_skips INIT(10);
extern bool g_next_frame();
extern bool g_last_frame();

class pover_video: public video {
    void on_process();
public:
#ifdef _WINDOWS
    bool graphic_display(){return video::win_hInstance != (HINSTANCE)NULL;}
#else
    bool graphic_display() { return true;} // fix this for Linux
#endif
    //void on_key(int key);
};

DEFINE int g_xwinsize INIT(1024);
DEFINE int g_ywinsize INIT(768);

DEFINE int map1XLoc INIT(10);
DEFINE int map1YLoc INIT(10);
DEFINE int map2XLoc INIT(270);
DEFINE int map2YLoc INIT(10);
DEFINE int maprXLoc INIT(530);
DEFINE int maprYLoc INIT(10);

DEFINE const char *g_windowTitle INIT("Polygon Overlay");
DEFINE bool g_useGraphics INIT(true);

extern bool initializeVideo(int argc, char **argv);

extern void rt_sleep(int msec);

#endif  // _POVER_VIDEO_H_
