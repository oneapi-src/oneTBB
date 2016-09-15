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

#ifndef FRACTAL_VIDEO_H_
#define FRACTAL_VIDEO_H_

#include "../../common/gui/video.h"
#include "fractal.h"

extern video *v;
extern bool single;

class fractal_video : public video
{
    fractal_group *fg;

private:
    void on_mouse( int x, int y, int key ) {
        if( key == 1 ) {
            if ( fg ) {
                fg->set_num_frames_at_least(20);
                fg->mouse_click( x, y );
            }
        }
    }

    void on_key( int key ) {
        switch ( key&0xff ) {
        case 27:
            running = false; break;
        case ' ': // space
            if( fg ) fg->switch_priorities();
        default:
            if( fg ) fg->set_num_frames_at_least(20);
        }
    }

    void on_process() {
        if ( fg ) {
            fg->run( !single );
        }
    }

public:
    fractal_video() :fg(0) {
        title = "Dynamic Priorities in TBB: Fractal Example";
        v = this;
    }

    void set_fractal_group( fractal_group &_fg ) {
        fg = &_fg;
    }
};

#endif /* FRACTAL_VIDEO_H_ */
