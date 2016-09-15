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

#ifndef FRACTAL_H_
#define FRACTAL_H_

#include "../../common/gui/video.h"

#include "tbb/task.h"
#include "tbb/task_scheduler_init.h"
#include "tbb/atomic.h"

//! Fractal class
class fractal {
    //! Left corner of the fractal area
    int off_x, off_y;
    //! Size of the fractal area
    int size_x, size_y;

    //! Fractal properties
    float cx, cy;
    float magn;
    int max_iterations;

    //! Drawing memory object for rendering
    const drawing_memory &dm;

    //! One pixel calculation routine
    color_t calc_one_pixel(int x, int y);
    //! Clears the fractal area
    void clear();
    //! Draws the border around the fractal area
    void draw_border( bool is_active );
    //! Renders the fractal
    void render( tbb::task_group_context &context );
    //! Check if the point is inside the fractal area
    bool check_point( int x, int y);

public:
    //! Constructor
    fractal( const drawing_memory &dm ) : dm(dm) {
#if _MSC_VER && _WIN64 && !__INTEL_COMPILER
        // Workaround for MSVC x64 compiler issue
        volatile int i=0;
#endif
    }
    //! Runs the fractal calculation
    void run( tbb::task_group_context &context );
    //! Renders the fractal rectangular area
    void render_rect(int x0, int y0, int x1, int y1);

    friend class fractal_group;
};

//! The group of fractals
class fractal_group {
    //! Fractals defenition
    fractal f0, f1;
    //! Number of frames to calculate
    tbb::atomic<int> num_frames[2];
    //! Task group contexts to manage prioroties
    tbb::task_group_context *context;

    //! Border type enumeration
    enum BORDER_TYPE {
        BORDER_INACTIVE = 0,
        BORDER_ACTIVE
    };

    //! The number of the threads
    int num_threads;
    //! The active (high priority) fractal number
    int active;

    //! Draws the borders around the fractals
    void draw_borders();
    //! Sets priorities for fractals calculations
    void set_priorities();

public:
    //! Constructor
    fractal_group( const drawing_memory &_dm, int num_threads = tbb::task_scheduler_init::automatic, int max_iterations = 100000, int num_frames = 1);
    //! Run calculation
    void run( bool create_second_fractal=true );
    //! Mouse event handler
    void mouse_click(int x, int y);
    //! Fractal calculation routine
    void calc_fractal( int num );
    //! Get number of threads
    int get_num_threads() const { return num_threads; }
    //! Reset the number of frames to be not less than the given value
    void set_num_frames_at_least(int n);
    //! Switches the priorities of two fractals
    void switch_priorities( int new_active=-1 );
};

#endif /* FRACTAL_H_ */
