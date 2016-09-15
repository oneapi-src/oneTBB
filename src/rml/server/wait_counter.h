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

#ifndef __RML_wait_counter_H
#define __RML_wait_counter_H

#include "thread_monitor.h"
#include "tbb/atomic.h"

namespace rml {
namespace internal {

class wait_counter {
    thread_monitor my_monitor;
    tbb::atomic<int> my_count;
    tbb::atomic<int> n_transients;
public:
    wait_counter() { 
        // The "1" here is subtracted by the call to "wait".
        my_count=1;
        n_transients=0;
    }

    //! Wait for number of operator-- invocations to match number of operator++ invocations.
    /** Exactly one thread should call this method. */
    void wait() {
        int k = --my_count;
        __TBB_ASSERT( k>=0, "counter underflow" );
        if( k>0 ) {
            thread_monitor::cookie c;
            my_monitor.prepare_wait(c);
            if( my_count )
                my_monitor.commit_wait(c);
            else 
                my_monitor.cancel_wait();
        }
        while( n_transients>0 )
            __TBB_Yield();
    }
    void operator++() {
        ++my_count;
    }
    void operator--() {
        ++n_transients;
        int k = --my_count;
        __TBB_ASSERT( k>=0, "counter underflow" );
        if( k==0 ) 
            my_monitor.notify();
        --n_transients;
    }
};

} // namespace internal
} // namespace rml

#endif /* __RML_wait_counter_H */
