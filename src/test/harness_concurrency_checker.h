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

#ifndef tbb_tests_harness_concurrency_checker_H
#define tbb_tests_harness_concurrency_checker_H

#include "harness_assert.h"
#include "harness_barrier.h"
#include "tbb/atomic.h"
#include "tbb/mutex.h"
#include "tbb/task.h"

namespace Harness {

/* note that if concurrency level is below than expected,
   the execution takes WAIT_SEC seconds */
class ConcurrencyChecker : public tbb::task {
public:
    struct State {
        Harness::SpinBarrier barrier;
        Harness::SpinBarrier join;
        const double         wait_sec;
        tbb::atomic<bool>    result;

        State(unsigned nthreads, double w_s)
            : barrier(nthreads), join(nthreads), wait_sec(w_s) {
            result = true;
        }
        void check() {
            if( !barrier.timed_wait_noerror(wait_sec) )
                result = false;
        }
    };

    ConcurrencyChecker(State *s) : st(s) { }

    tbb::task* execute() {
        if( st->result ) // skip if the time-out is detected already
            st->check();
        st->join.signal_nowait();
        return NULL;
    }
private:
    State *st;
};

bool CanReachConcurrencyLevel(int conc_level, double wait_timeout_sec = 30.)
{
    static tbb::mutex global_mutex;
    // prevent concurrent calls to this function from deadlock due to the barrier
    tbb::mutex::scoped_lock lock(global_mutex);
    ConcurrencyChecker::State state(conc_level, wait_timeout_sec);

    for( int i=1; i<conc_level; i++)
        tbb::task::enqueue( *new(tbb::task::allocate_root()) ConcurrencyChecker(&state) );
    state.check();
    state.join.wait();
    return state.result;
}

} // namespace Harness

#endif /* tbb_tests_harness_concurrency_checker_H */
