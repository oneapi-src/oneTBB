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

#ifndef SEISMIC_VIDEO_H_
#define SEISMIC_VIDEO_H_

#include "../../common/gui/video.h"

class Universe;

class SeismicVideo : public video
{
#ifdef _WINDOWS
    #define MAX_LOADSTRING 100
    TCHAR szWindowClass[MAX_LOADSTRING];    // the main window class name
    WNDCLASSEX wcex;
#endif
    static const char * const titles[2];

    bool initIsParallel ;

    Universe &u_;
    int numberOfFrames_; // 0 means forever, positive means number of frames, negative is undefined
    int threadsHigh;
private:
    void on_mouse(int x, int y, int key);
    void on_process();

#ifdef _WINDOWS
public:
#endif
    void on_key(int key);

public:
    SeismicVideo(    Universe &u,int numberOfFrames, int threadsHigh, bool initIsParallel=true);
};
#endif /* SEISMIC_VIDEO_H_ */
