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

// Test that __TBB_Yield works.
// On Red Hat EL4 U1, it does not work, because sched_yield is broken.

#define HARNESS_DEFAULT_MIN_THREADS 4
#define HARNESS_DEFAULT_MAX_THREADS 8

#include "tbb/tbb_machine.h"
#include "tbb/tick_count.h"
#include "harness.h"

static volatile long CyclicCounter;
static volatile bool Quit;
double SingleThreadTime;

struct RoundRobin: NoAssign {
    const int number_of_threads;
    RoundRobin( long p ) : number_of_threads(p) {}
    void operator()( long k ) const {
        tbb::tick_count t0 = tbb::tick_count::now();
        for( long i=0; i<10000; ++i ) {
            // Wait for previous thread to notify us
            for( int j=0; CyclicCounter!=k && !Quit; ++j ) {
                __TBB_Yield();
                if( j%100==0 ) {
                    tbb::tick_count t1 = tbb::tick_count::now();
                    if( (t1-t0).seconds()>=1.0*number_of_threads ) {
                        REPORT("Warning: __TBB_Yield failing to yield with %d threads (or system is heavily loaded)\n",number_of_threads);
                        Quit = true;
                        return;
                    }
                }
            }
            // Notify next thread that it can run
            CyclicCounter = (k+1)%number_of_threads;
        }
    }
};

int TestMain () {
    for( int p=MinThread; p<=MaxThread; ++p ) {
        REMARK("testing with %d threads\n", p );
        CyclicCounter = 0;
        Quit = false;
        NativeParallelFor( long(p), RoundRobin(p) );
    }
    return Harness::Done;
}

