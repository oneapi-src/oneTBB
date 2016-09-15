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

#define HARNESS_DEFINE_PRIVATE_PUBLIC 1
#include "harness_inject_scheduler.h"
#include "harness.h"

#define TBB_PREVIEW_GLOBAL_CONTROL 1
#include "tbb/global_control.h"
#include "tbb/task_scheduler_init.h"
#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"

bool allWorkersSleep() {
    using namespace tbb::internal;
    using namespace tbb::internal::rml;

    unsigned sleeping_threads = 0;
    unsigned threads = ((private_server*)market::theMarket->my_server)->my_n_thread;

    for (private_worker *l = ((private_server*)market::theMarket->my_server)->my_asleep_list_root;
         l; l = l->my_next)
        sleeping_threads++;

    return threads == sleeping_threads;
}

class ThreadsTask {
public:
    void operator() (const tbb::blocked_range<int> &) const { }
    ThreadsTask() {}
};

static void RunAndCheckSleeping()
{
    Harness::Sleep(100);
    ASSERT(allWorkersSleep(), NULL);
    tbb::parallel_for(tbb::blocked_range<int>(0, 100*1000, 1),
                      ThreadsTask(), tbb::simple_partitioner());
    Harness::Sleep(100);
    ASSERT(allWorkersSleep(), NULL);
}

// test that all workers are sleeping, not spinning
void TestWorkersSleep() {
    tbb::task_scheduler_init tsi(8);
    const size_t max_parallelism =
        tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism);
    if (max_parallelism > 2) {
        tbb::global_control c(tbb::global_control::max_allowed_parallelism, max_parallelism-1);
    }
    RunAndCheckSleeping();
    tbb::global_control c(tbb::global_control::max_allowed_parallelism, max_parallelism+1);
    RunAndCheckSleeping();
}

int TestMain () {
    {
        tbb::task_scheduler_init tsi;
        if (!tbb::internal::governor::UsePrivateRML)
            return Harness::Skipped;
    }
    TestWorkersSleep();

    return Harness::Done;
}
