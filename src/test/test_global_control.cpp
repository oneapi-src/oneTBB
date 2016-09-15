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

#define TBB_PREVIEW_WAITING_FOR_WORKERS 1
#define TBB_PREVIEW_GLOBAL_CONTROL 1
#include "tbb/global_control.h"
#include "harness.h"

const size_t MB = 1024*1024;
const double BARRIER_TIMEOUT = 10.;

void TestStackSizeSimpleControl()
{
    {
        tbb::global_control s0(tbb::global_control::thread_stack_size, 1*MB);

        {
            tbb::global_control s1(tbb::global_control::thread_stack_size, 8*MB);

            ASSERT(8*MB == tbb::global_control::active_value(tbb::global_control::thread_stack_size), NULL);
        }
        ASSERT(1*MB == tbb::global_control::active_value(tbb::global_control::thread_stack_size), NULL);
    }
}

#include "harness_concurrency_checker.h"
#include "tbb/parallel_for.h"
#include "tbb/task_scheduler_init.h"
#include "tbb/blocked_range.h"
#include "tbb/combinable.h"
#include <limits.h>   // for UINT_MAX
#include <functional> // for std::plus

struct StackSizeRun: NoAssign {
    int                   num_threads;
    Harness::SpinBarrier *barr1, *barr2;

    StackSizeRun(int threads, Harness::SpinBarrier *b1, Harness::SpinBarrier *b2) :
        num_threads(threads), barr1(b1), barr2(b2) {}
    void operator()( int id ) const {
        tbb::global_control s1(tbb::global_control::thread_stack_size, (1+id)*MB);

        barr1->timed_wait(BARRIER_TIMEOUT);

        ASSERT(num_threads*MB == tbb::global_control::active_value(tbb::global_control::thread_stack_size), NULL);
        barr2->timed_wait(BARRIER_TIMEOUT);
    }
};

void TestStackSizeThreadsControl()
{
    int threads = 4;
    Harness::SpinBarrier barr1(threads), barr2(threads);
    NativeParallelFor( threads, StackSizeRun(threads, &barr1, &barr2) );
}

class CheckWorkersNum {
    static tbb::atomic<Harness::SpinBarrier*> barrier;
    // count unique worker threads
    static tbb::combinable<size_t>            uniqThreads;
public:
    CheckWorkersNum(Harness::SpinBarrier *barr) {
        barrier = barr;
    }
    void operator()(const tbb::blocked_range<int>&) const {
        uniqThreads.local() = 1;
        if (barrier) {
            barrier->timed_wait(BARRIER_TIMEOUT);
            Harness::Sleep(10);
            barrier = NULL;
        }
    }
    static void check(size_t expected) {
        size_t seen = uniqThreads.combine(std::plus<size_t>());
        ASSERT(seen == expected, NULL);
    }
    static void clear() { uniqThreads.clear(); }
    static const size_t LOOP_ITERS = 10*1000;
};

tbb::atomic<Harness::SpinBarrier*> CheckWorkersNum::barrier;
tbb::combinable<size_t>  CheckWorkersNum::uniqThreads;

void RunWorkersLimited(int tsi_max_threads, size_t parallelism, bool wait)
{
    tbb::global_control s(tbb::global_control::max_allowed_parallelism, parallelism);
    // try both configuration with already sleeping workers and with not yet sleeping
    if (wait)
        Harness::Sleep(100);
    // current implementation can't have effective active value below 2
    const unsigned active_parallelism = max(2U, (unsigned)parallelism);
    const unsigned expected_threads = tsi_max_threads>0?
        min( (unsigned)tsi_max_threads, active_parallelism )
        : ( tbb::tbb_thread::hardware_concurrency()==1? 1 : active_parallelism );
    Harness::SpinBarrier barr(expected_threads);

    CheckWorkersNum::clear();
    tbb::parallel_for(tbb::blocked_range<int>(0, CheckWorkersNum::LOOP_ITERS, 1),
                      CheckWorkersNum(&barr), tbb::simple_partitioner());
    CheckWorkersNum::check(expected_threads);
}

void TSI_and_RunWorkers(int tsi_max_threads, size_t parallelism, size_t max_value)
{
    tbb::task_scheduler_init tsi(tsi_max_threads, 0, /*blocking=*/true);
    size_t active = tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism);
    ASSERT(active == max(2U, max_value), "active_value must not be changed by task_scheduler_init");
    RunWorkersLimited(tsi_max_threads, parallelism, /*wait=*/false);
}

#include "tbb/tbb_thread.h"

void TestWorkers(size_t curr_par)
{
    const size_t max_parallelism =
        tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism);
    ASSERT(max(2U, tbb::tbb_thread::hardware_concurrency()) == max_parallelism, NULL);
    {
        const unsigned h_c = tbb::tbb_thread::hardware_concurrency();
        tbb::global_control c(tbb::global_control::max_allowed_parallelism, curr_par);
        size_t v = tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism);
        ASSERT(!curr_par || max((size_t)2, curr_par) == v, NULL);
        if (h_c > 1)
            TSI_and_RunWorkers(tbb::task_scheduler_init::automatic, min(h_c, curr_par), curr_par);
        if (curr_par) // do not call task_scheduler_init t(0);
            TSI_and_RunWorkers((int)curr_par, curr_par, curr_par);
        if (curr_par > 2) { // check that min(tsi, parallelism) is active
            TSI_and_RunWorkers((int)curr_par-1, curr_par, curr_par);
            TSI_and_RunWorkers((int)curr_par, curr_par-1, curr_par);
        }
        // check constrains on control's value: it can't be increased
        tbb::global_control c1(tbb::global_control::max_allowed_parallelism, curr_par+1);
        v = tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism);
        if (curr_par)
            ASSERT(max(2U, curr_par) == v, "It's impossible to increase maximal parallelism.");
        else
            ASSERT(2 == v, NULL);
    }
    ASSERT(tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism)
           == max_parallelism,
           "max parallelism has been restored successfully after decreasing/increasing");
}

void TestWorkersConstraints() {
    const size_t max_parallelism =
        tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism);
    tbb::task_scheduler_init tsi(tbb::task_scheduler_init::automatic, 0, /*blocking=*/true);
    if (max_parallelism > 3) {
        tbb::global_control c(tbb::global_control::max_allowed_parallelism, max_parallelism-1);
        ASSERT(max_parallelism-1 ==
               tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism),
               "Allowed parallelism must be decreasable.");
        tbb::global_control c1(tbb::global_control::max_allowed_parallelism, max_parallelism-2);
        ASSERT(max_parallelism-2 ==
               tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism),
               "Allowed parallelism must be decreasable.");
    }
    const size_t limit_par = min(max_parallelism, 4U);
    // check that constrains are really met
    for (int wait=0; wait<2; wait++) {
        for (size_t num=2; num<limit_par; num++)
            RunWorkersLimited(tbb::task_scheduler_init::automatic, num, wait==1);
        for (size_t num=limit_par; num>1; num--)
            RunWorkersLimited(tbb::task_scheduler_init::automatic, num, wait==1);
    }
}

struct SetUseRun: NoAssign {
    Harness::SpinBarrier *barr;

    SetUseRun(Harness::SpinBarrier *b) : barr(b) {}
    void operator()( int id ) const {
        if (id == 0) {
            for (int i=0; i<10; i++) {
                tbb::task_scheduler_init tsi(tbb::task_scheduler_init::automatic, 0,
                                             /*blocking=*/true);
                tbb::parallel_for(tbb::blocked_range<int>(0, CheckWorkersNum::LOOP_ITERS, 1),
                                  CheckWorkersNum(NULL), tbb::simple_partitioner());
                barr->timed_wait(BARRIER_TIMEOUT);
            }
        } else {
            for (int i=0; i<10; i++) {
                tbb::global_control c(tbb::global_control::max_allowed_parallelism, 8);
                barr->timed_wait(BARRIER_TIMEOUT);
            }
        }
    }
};

void TestConcurrentSetUseConcurrency()
{
    Harness::SpinBarrier barr(2);
    NativeParallelFor( 2, SetUseRun(&barr) );
}

// check number of workers after autoinitialization
void TestAutoInit()
{
    const size_t max_parallelism =
        tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism);
    const unsigned expected_threads = tbb::tbb_thread::hardware_concurrency()==1?
        1 : (unsigned)max_parallelism;
    Harness::SpinBarrier barr(expected_threads);

    CheckWorkersNum::clear();
    tbb::parallel_for(tbb::blocked_range<int>(0, CheckWorkersNum::LOOP_ITERS, 1),
                      CheckWorkersNum(&barr), tbb::simple_partitioner());
    ASSERT(tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism)
           == max_parallelism, "max_allowed_parallelism must not be changed after auto init");
    CheckWorkersNum::check(expected_threads);
    if (max_parallelism > 2) {
        // after autoinit it's possible to decrease workers number
        tbb::global_control s(tbb::global_control::max_allowed_parallelism, max_parallelism-1);
        const unsigned expected_threads_1 = max(2U, (unsigned)max_parallelism-1);
        barr.initialize(expected_threads_1);
        CheckWorkersNum::clear();
        tbb::parallel_for(tbb::blocked_range<int>(0, CheckWorkersNum::LOOP_ITERS, 1),
                          CheckWorkersNum(&barr), tbb::simple_partitioner());
        CheckWorkersNum::check(expected_threads_1);
    }
}

// need this to use TRY_BAD_EXPR_ENABLED when TBB_USE_ASSERT is not defined
#undef TBB_USE_ASSERT
#define TBB_USE_ASSERT 1

#include "harness_bad_expr.h"

void TestInvalidParallelism()
{
#if TRY_BAD_EXPR_ENABLED
    const size_t max_parallelism =
        tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism);
    for (size_t par = 0; par<=1; par++) {
        {
            tbb::set_assertion_handler( AssertionFailureHandler );
            TRY_BAD_EXPR( tbb::global_control c(tbb::global_control::max_allowed_parallelism, par), "Values of 1 and 0 are not supported for max_allowed_parallelism." );
            tbb::set_assertion_handler( ReportError );
            ASSERT(tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism)
                   == max_parallelism, NULL);
        }
        {
            const size_t P = 2;
            tbb::global_control c(tbb::global_control::max_allowed_parallelism, P);
            ASSERT(tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism)
                   == P, NULL);
            tbb::set_assertion_handler( AssertionFailureHandler );
            TRY_BAD_EXPR( tbb::global_control cZ(tbb::global_control::max_allowed_parallelism, par), "Values of 1 and 0 are not supported for max_allowed_parallelism." );
            tbb::set_assertion_handler( ReportError );
            ASSERT(tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism)
                   == P, NULL);
        }
        ASSERT(tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism)
               == max_parallelism, NULL);
    }
#endif /* TRY_BAD_EXPR_ENABLED */
}

void TestTooBigStack()
{
#if __TBB_x86_32
    const size_t stack_sizes[] = {512*MB, 2*1024*MB, UINT_MAX};
#else
    const size_t stack_sizes[] = {512*MB, 2*1024*MB, UINT_MAX, 10LU*1024*MB};
#endif

#if __TBB_WIN8UI_SUPPORT
    size_t default_ss = tbb::global_control::active_value(tbb::global_control::thread_stack_size);
#endif
    for (unsigned i = 0; i<Harness::array_length(stack_sizes); i++) {
        // as no stack size setting for Windows Store* apps, skip it
#if TRY_BAD_EXPR_ENABLED && __TBB_x86_64 && (_WIN32 || _WIN64) && !__TBB_WIN8UI_SUPPORT
        if (stack_sizes[i] != (unsigned)stack_sizes[i]) {
            size_t curr_ss = tbb::global_control::active_value(tbb::global_control::thread_stack_size);
            tbb::set_assertion_handler( AssertionFailureHandler );
            TRY_BAD_EXPR( tbb::global_control s1(tbb::global_control::thread_stack_size, stack_sizes[i]), "Stack size is limited to unsigned int range" );
            tbb::set_assertion_handler( ReportError );
            ASSERT(curr_ss == tbb::global_control::active_value(tbb::global_control::thread_stack_size), "Changing of stack size is not expected.");
            continue;
        }
#endif
        tbb::global_control s1(tbb::global_control::thread_stack_size, stack_sizes[i]);
        size_t actual_stack_sz = tbb::global_control::active_value(tbb::global_control::thread_stack_size);
#if __TBB_WIN8UI_SUPPORT
        ASSERT(actual_stack_sz == default_ss, "It's ignored for Windows Store* apps");
#else
        ASSERT(actual_stack_sz==stack_sizes[i], NULL);
#endif
    }
}

int TestMain()
{
    const unsigned h_c = tbb::tbb_thread::hardware_concurrency();
    bool excessHC;
    {
        tbb::task_scheduler_init t(h_c+1);
        excessHC = Harness::CanReachConcurrencyLevel(h_c+1);
    }
    if (h_c>2)
        TestWorkers(h_c-1);
    if (excessHC)  // this requires hardware concurrency +1, and hang if not provided
        TestWorkers(h_c+1);
    if (excessHC || h_c >= 2)
        TestWorkers(2);
    if (excessHC || h_c >= 3)
        TestWorkers(3);
    TestWorkersConstraints();
    TestConcurrentSetUseConcurrency();
    TestInvalidParallelism();
    TestAutoInit(); // auto-initialization done at this point

    size_t default_ss = tbb::global_control::active_value(tbb::global_control::thread_stack_size);
    ASSERT(default_ss, NULL);
#if !__TBB_WIN8UI_SUPPORT
    // it's impossible to change stack size for Windows Store* apps, so skip the tests
    TestStackSizeSimpleControl();
    TestStackSizeThreadsControl();
#endif
    TestTooBigStack();
    ASSERT(default_ss == tbb::global_control::active_value(tbb::global_control::thread_stack_size), NULL);

    return Harness::Done;
}
