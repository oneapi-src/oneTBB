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

#include "tbb/atomic.h"
#include "tbb/tick_count.h"

#ifndef harness_barrier_H
#define harness_barrier_H

namespace Harness {

//! Spin WHILE the value of the variable is equal to a given value
/** T and U should be comparable types. */
class TimedWaitWhileEq {
    //! Assignment not allowed
    void operator=( const TimedWaitWhileEq& );
    double &my_limit;
public:
    TimedWaitWhileEq(double &n_seconds) : my_limit(n_seconds) {}
    TimedWaitWhileEq(const TimedWaitWhileEq &src) : my_limit(src.my_limit) {}
    template<typename T, typename U>
    void operator()( const volatile T& location, U value ) const {
        tbb::tick_count start = tbb::tick_count::now();
        double time_passed;
        do {
            time_passed = (tbb::tick_count::now()-start).seconds();
            if( time_passed < 0.0001 ) __TBB_Pause(10); else __TBB_Yield();
        } while( time_passed < my_limit && location == value);
        my_limit -= time_passed;
    }
};
//! Spin WHILE the value of the variable is equal to a given value
/** T and U should be comparable types. */
class WaitWhileEq {
    //! Assignment not allowed
    void operator=( const WaitWhileEq& );
public:
    template<typename T, typename U>
    void operator()( const volatile T& location, U value ) const {
        tbb::internal::spin_wait_while_eq(location, value);
    }
};
class SpinBarrier
{
    unsigned numThreads;
    tbb::atomic<unsigned> numThreadsFinished; /* threads reached barrier in this epoch */
    tbb::atomic<unsigned> epoch;   /* how many times this barrier used - XXX move to a separate cache line */

    struct DummyCallback {
        void operator() () const {}
        template<typename T, typename U>
        void operator()( const T&, U) const {}
    };

    SpinBarrier( const SpinBarrier& );    // no copy ctor
    void operator=( const SpinBarrier& ); // no assignment
public:
    SpinBarrier( unsigned nthreads = 0 ) { initialize(nthreads); };

    void initialize( unsigned nthreads ) {
        numThreads = nthreads;
        numThreadsFinished = 0;
        epoch = 0;
    };

    // onOpenBarrierCallback is called by last thread arrived on a barrier
    template<typename WaitEq, typename Callback>
    bool custom_wait(const WaitEq &onWaitCallback, const Callback &onOpenBarrierCallback)
    { // return true if last thread
        unsigned myEpoch = epoch;
        unsigned myNumThreads = numThreads; // read it before the increment
        int threadsLeft = myNumThreads - numThreadsFinished.fetch_and_increment() - 1;
        ASSERT(threadsLeft>=0, "Broken barrier");
        if (threadsLeft > 0) {
            /* not the last threading reaching barrier, wait until epoch changes & return 0 */
            onWaitCallback(epoch, myEpoch);
            return false;
        }
        onOpenBarrierCallback();
        /* No more threads left to enter, so I'm the last one reaching this epoch;
           reset the barrier, increment epoch, and return non-zero */
        threadsLeft = numThreadsFinished -= myNumThreads;
        ASSERT( threadsLeft == 0, "Broken barrier");
        /* wakes up threads waiting to exit this epoch */
        myEpoch -= epoch++;
        ASSERT( myEpoch == 0, "Broken barrier");
        return true;
    }
    bool timed_wait_noerror(double n_seconds) {
        custom_wait(TimedWaitWhileEq(n_seconds), DummyCallback());
        return n_seconds >= 0.0001;
    }
    bool timed_wait(double n_seconds, const char *msg="Time is out while waiting on a barrier") {
        bool is_last = custom_wait(TimedWaitWhileEq(n_seconds), DummyCallback());
        ASSERT( n_seconds >= 0, msg); // TODO: refactor to avoid passing msg here and rising assertion
        return is_last;
    }
    //! onOpenBarrierCallback is called by last thread arrived on a barrier
    template<typename Callback>
    bool wait(const Callback &onOpenBarrierCallback) {
        return custom_wait(WaitWhileEq(), onOpenBarrierCallback);
    }
    bool wait(){
        return wait(DummyCallback());
    }
    //! signal to the barrier, rather a semaphore functionality
    bool signal_nowait() {
        return custom_wait(DummyCallback(),DummyCallback());
    }
};

}

#endif //harness_barrier_H
