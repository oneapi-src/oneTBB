/*
    Copyright 2005-2015 Intel Corporation.  All Rights Reserved.

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

#include "machine.h"
#include "types.h"
#include "macros.h"
#include "vector.h"
#include "tgafile.h"
#include "trace.h"
#include "light.h"
#include "shade.h"
#include "camera.h"
#include "util.h"
#include "intersect.h"
#include "global.h"
#include "ui.h"
#include "tachyon_video.h"

// shared but read-only so could be private too
static thr_parms *all_parms;
static scenedef scene;
static int startx;
static int stopx;
static int starty;
static int stopy;
static flt jitterscale;
static int totaly;

static color_t render_one_pixel (int x, int y, unsigned int *local_mbox, unsigned int &serial,
                                 int startx, int stopx, int starty, int stopy)
{
    /* private vars moved inside loop */
    ray primary, sample;
    color col, avcol;
    int R,G,B;
    intersectstruct local_intersections;
    int alias;
    /* end private */

    primary = camray(&scene, x, y);
    primary.intstruct = &local_intersections;
    primary.flags = RT_RAY_REGULAR;

    serial++;
    primary.serial = serial;
    primary.mbox = local_mbox;
    primary.maxdist = FHUGE;
    primary.scene = &scene;
    col = trace(&primary);
    serial = primary.serial;

    /* Handle overexposure and underexposure here... */
    R = (int)(col.r * 255);
    if ( R > 255 ) R = 255;
    else if ( R < 0 ) R = 0;

    G = (int)(col.g * 255);
    if ( G > 255 ) G = 255;
    else if ( G < 0 ) G = 0;

    B = (int)(col.b * 255);
    if ( B > 255 ) B = 255;
    else if ( B < 0 ) B = 0;

    return video->get_color(R, G, B);
}

#if DO_ITT_NOTIFY
#include"ittnotify.h"
#endif

#define RUNTIME_SERIAL 1
#define RUNTIME_OPENMP 2
#define RUNTIME_CILK   3
#define RUNTIME_TBB    4

#ifndef RUNTIME
#define RUNTIME RUNTIME_TBB
#endif

#if RUNTIME == RUNTIME_OPENMP
#include <omp.h>
#elif RUNTIME == RUNTIME_TBB
#include <tbb/tbb.h>
#endif

static void parallel_thread(void)
{
    unsigned int mboxsize = sizeof(unsigned int)*(max_objectid() + 20);
#if RUNTIME == RUNTIME_SERIAL
    for ( int y = starty; y < stopy; y++ )
#elif RUNTIME == RUNTIME_OPENMP
#pragma omp parallel for
    for ( int y = starty; y < stopy; y++ )
#elif RUNTIME == RUNTIME_CILK
    _Cilk_for(int y = starty; y < stopy; y++)
#elif RUNTIME == RUNTIME_TBB
    tbb::parallel_for(starty, stopy, [mboxsize] (int y)
#endif
    {
        unsigned int serial = 1;
        unsigned int local_mbox[mboxsize];
        memset(local_mbox, 0, mboxsize);
        drawing_area drawing(startx, totaly - y, stopx - startx, 1);
        for ( int x = startx; x < stopx; x++ ) {
            color_t c = render_one_pixel(x, y, local_mbox, serial, startx, stopx, starty, stopy);
            drawing.put_pixel(c);
        }
        video->next_frame();
    }
#if RUNTIME == RUNTIME_TBB
    );
#endif
}

void * thread_trace(thr_parms * parms)
{
    // shared but read-only so could be private too
    all_parms = parms;
    scene = parms->scene;
    startx = parms->startx;
    stopx = parms->stopx;
    starty = parms->starty;
    stopy = parms->stopy;
    jitterscale = 40.0*(scene.hres + scene.vres);
    totaly = parms->scene.vres - 1;

#if DO_ITT_NOTIFY
    __itt_resume();
#endif
    parallel_thread();
#if DO_ITT_NOTIFY
    __itt_pause();
#endif

    return(NULL);
}
