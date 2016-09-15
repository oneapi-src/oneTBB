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

#include "harness.h"
#if __TBB_MIC_OFFLOAD
int TestMain () {
    return Harness::Skipped;
}
#else
#include "job_automaton.h"
#define HARNESS_NO_PARSE_COMMAND_LINE 1
#include "harness_barrier.h"

class State {
    Harness::SpinBarrier barrier;
    rml::internal::job_automaton ja;
    rml::job job;
    tbb::atomic<int> job_created;
    tbb::atomic<int> job_destroyed;
    tbb::atomic<bool> job_received;
public:
    State() : barrier(2) {
        job_created = 0;
        job_destroyed = 0;
        job_received = false;
    }
    void exercise( bool is_owner );
    ~State() {
        ASSERT( job_created==job_destroyed, "accounting error" );
        ASSERT( job_destroyed<=1, "destroyed job twice" );
    }
};

int DelayMask;
const int N = 14; 
tbb::atomic<int> Coverage[N];

//! Mark kth interval as covered and insert delay if kth bit of DelayMask is set.
/** An interval is the code between two operations on the job_automaton that we are testing. */
void Cover( int k ) {
    ASSERT( k<N, NULL );
    ++Coverage[k];
    if( DelayMask>>k&1 ) {
        // Introduce delay (and possibly a thread context switch)
        __TBB_Yield();
    }
}

void State::exercise( bool is_owner ) {
    barrier.wait();
    if( is_owner ) {
        Cover(0);
        if( ja.try_acquire() ) {
            Cover(1);
            ++job_created; 
            ja.set_and_release(job);
            Cover(2);
            if( ja.try_acquire() ) {
                Cover(3);
                ja.release();
                Cover(4);
                if( ja.try_acquire() ) {
                    Cover(5);
                    ja.release();
                }
            }
            Cover(6);
        } else {
            Cover(7);
        }
        if( DelayMask&1<<N ) {
            while( !job_received ) 
                __TBB_Yield();
        }
    } else {
        // Using extra bit of DelayMask for choosing whether to run wait_for_job or not.
        if( DelayMask&1<<N ) {
            rml::job* j= &ja.wait_for_job(); 
            if( j!=&job ) REPORT("%p\n",j);
            ASSERT( j==&job, NULL );
            job_received = true;
        }
        Cover(8);
    }   
    rml::job* j;
    if( ja.try_plug(j) ) {
        ASSERT( j==&job || !j, NULL );
        if( j ) {
            Cover(9+is_owner);
            ++job_destroyed;
        } else {
            __TBB_ASSERT( !is_owner, "owner failed to create job but plugged self" );
            Cover(11);
        } 
    } else {
        Cover(12+is_owner);
    }
}

class Loop: NoAssign {
    State& s;
public:
    Loop(State& s_) : s(s_) {}
    void operator()( int i ) const {s.exercise(i==0);}
};

/** Return true if coverage is acceptable.
    If report==true, issue message if it is unacceptable. */
bool CheckCoverage( bool report ) {
    bool okay = true;
    for( int i=0; i<N; ++i ) {
        const int min_coverage = 4; 
        if( Coverage[i]<min_coverage ) {
            okay = false;
            if( report )
                REPORT("Warning: Coverage[%d]=%d is less than acceptable minimum of %d\n", i, int(Coverage[i]),min_coverage);
        }
    }
    return okay;
}

int TestMain () {
    for( DelayMask=0; DelayMask<8<<N; ++DelayMask ) {
        State s;
        NativeParallelFor( 2, Loop(s) );
        if( CheckCoverage(false) ) { 
            // Reached acceptable code coverage level
            break;
        }
    }
    CheckCoverage(true);
    return Harness::Done;
}

#endif /* __TBB_MIC_OFFLOAD */
