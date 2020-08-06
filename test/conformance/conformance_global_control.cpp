/*
    Copyright (c) 2005-2020 Intel Corporation

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/
#include "common/test.h"
#include "common/concurrency_tracker.h"
#include "common/spin_barrier.h"
#include "common/utils.h"
#include "common/utils_concurrency_limit.h"

#include "tbb/global_control.h"
#include "tbb/parallel_for.h"

#include <limits.h>
#include <thread>

//! \file conformance_global_control.cpp
//! \brief Test for [sched.global_control] specification

const std::size_t MB = 1024*1024;
const double BARRIER_TIMEOUT = 10.;

void TestStackSizeSimpleControl() {
    tbb::global_control s0(tbb::global_control::thread_stack_size, 1*MB);

    {
        tbb::global_control s1(tbb::global_control::thread_stack_size, 8*MB);

        CHECK(8*MB == tbb::global_control::active_value(tbb::global_control::thread_stack_size));
    }
    CHECK(1*MB == tbb::global_control::active_value(tbb::global_control::thread_stack_size));
}

struct StackSizeRun : utils::NoAssign {

    int num_threads;
    utils::SpinBarrier *barr1, *barr2;

    StackSizeRun(int threads, utils::SpinBarrier *b1, utils::SpinBarrier *b2) :
        num_threads(threads), barr1(b1), barr2(b2) {}
    void operator()( int id ) const {
        tbb::global_control s1(tbb::global_control::thread_stack_size, (1+id)*MB);

        barr1->timedWaitNoError(BARRIER_TIMEOUT);

        REQUIRE(num_threads*MB == tbb::global_control::active_value(tbb::global_control::thread_stack_size));
        barr2->timedWaitNoError(BARRIER_TIMEOUT);
    }
};

void TestStackSizeThreadsControl() {
    int threads = 4;
    utils::SpinBarrier barr1(threads), barr2(threads);
    utils::NativeParallelFor( threads, StackSizeRun(threads, &barr1, &barr2) );
}

void RunWorkersLimited(size_t parallelism, bool wait)
{
    tbb::global_control s(tbb::global_control::max_allowed_parallelism, parallelism);
    // try both configuration with already sleeping workers and with not yet sleeping
    if (wait)
        utils::Sleep(100);
    const std::size_t expected_threads = (utils::get_platform_max_threads()==1)? 1 : parallelism;
    utils::ExactConcurrencyLevel::check(expected_threads);
}

void TestWorkersConstraints()
{
    const size_t max_parallelism =
        tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism);
    if (max_parallelism > 3) {
        tbb::global_control c(tbb::global_control::max_allowed_parallelism, max_parallelism-1);
        CHECK_MESSAGE(max_parallelism-1 ==
               tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism),
               "Allowed parallelism must be decreasable.");
        tbb::global_control c1(tbb::global_control::max_allowed_parallelism, max_parallelism-2);
        CHECK_MESSAGE(max_parallelism-2 ==
               tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism),
               "Allowed parallelism must be decreasable.");
    }
    const size_t limit_par = utils::min(max_parallelism, 4U);
    // check that constrains are really met
    for (int wait=0; wait<2; wait++) {
        for (size_t num=2; num<limit_par; num++)
            RunWorkersLimited(num, wait==1);
        for (size_t num=limit_par; num>1; num--)
            RunWorkersLimited(num, wait==1);
    }
}

void RunParallelWork() {
    const int LOOP_ITERS = 10*1000;
    tbb::parallel_for(0, LOOP_ITERS, [](int){ utils::Sleep(1); }, tbb::simple_partitioner());
}

struct SetUseRun: utils::NoAssign {
    utils::SpinBarrier &barr;

    SetUseRun(utils::SpinBarrier& b) : barr(b) {}
    void operator()( int id ) const {
        if (id == 0) {
            for (int i=0; i<10; i++) {
                RunParallelWork();
                barr.timedWaitNoError(BARRIER_TIMEOUT);
            }
        } else {
            for (int i=0; i<10; i++) {
                tbb::global_control c(tbb::global_control::max_allowed_parallelism, 8);
                barr.timedWaitNoError(BARRIER_TIMEOUT);
            }
        }
    }
};

void TestConcurrentSetUseConcurrency()
{
    utils::SpinBarrier barr(2);
    NativeParallelFor( 2, SetUseRun(barr) );
}

// check number of workers after autoinitialization
void TestAutoInit()
{
    const size_t max_parallelism =
        tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism);
    const unsigned expected_threads = utils::get_platform_max_threads()==1?
        1 : (unsigned)max_parallelism;
    utils::ExactConcurrencyLevel::check(expected_threads);
    CHECK_MESSAGE(tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism)
           == max_parallelism, "max_allowed_parallelism must not be changed after auto init");
    if (max_parallelism > 2) {
        // after autoinit it's possible to decrease workers number
        tbb::global_control s(tbb::global_control::max_allowed_parallelism, max_parallelism-1);
        utils::ExactConcurrencyLevel::check(max_parallelism-1);
    }
}

class TestMultipleControlsRun {
    utils::SpinBarrier &barrier;
public:
    TestMultipleControlsRun(utils::SpinBarrier& b) : barrier(b) {}
    void operator()( int id ) const {
        barrier.wait();
        if (id) {
            {
                tbb::global_control c(tbb::global_control::max_allowed_parallelism, 1);
                utils::ExactConcurrencyLevel::check(1);
                barrier.wait();
            }
            utils::ExactConcurrencyLevel::check(1);
            barrier.wait();
            {
                tbb::global_control c(tbb::global_control::max_allowed_parallelism, 2);
                utils::ExactConcurrencyLevel::check(1);
                barrier.wait();
                utils::ExactConcurrencyLevel::check(2);
                barrier.wait();
            }
        } else {
            {
                utils::ExactConcurrencyLevel::check(1);
                tbb::global_control c(tbb::global_control::max_allowed_parallelism, 1);
                barrier.wait();
                utils::ExactConcurrencyLevel::check(1);
                barrier.wait();
                utils::ExactConcurrencyLevel::check(1);
                barrier.wait();
            }
            utils::ExactConcurrencyLevel::check(2);
            barrier.wait();
        }
    }
};

// test that global controls from different thread with overlapping lifetime
// still keep parallelism under control
void TestMultipleControls()
{
    utils::SpinBarrier barrier(2);
    utils::NativeParallelFor( 2, TestMultipleControlsRun(barrier) );
}

#if !(__TBB_WIN8UI_SUPPORT && (_WIN32_WINNT < 0x0A00))
//! Testing setting stack size
//! \brief \ref interface \ref requirement
TEST_CASE("setting stack size") {
    std::size_t default_ss = tbb::global_control::active_value(tbb::global_control::thread_stack_size);
    CHECK(default_ss > 0);
    TestStackSizeSimpleControl();
    TestStackSizeThreadsControl();
    CHECK(default_ss == tbb::global_control::active_value(tbb::global_control::thread_stack_size));
}
#endif


//! Testing setting max number of threads
//! \brief \ref interface \ref requirement
TEST_CASE("setting max number of threads") {
    TestWorkersConstraints();
    TestConcurrentSetUseConcurrency();
    TestAutoInit();
}
