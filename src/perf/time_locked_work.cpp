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

////// Test configuration ////////////////////////////////////////////////////
#define SECONDS_RATIO 1000000 // microseconds

#ifndef REPEAT_K
#define REPEAT_K 50 // repeat coefficient
#endif

int outer_work[] = {/*256,*/ 64, 16, 4, 0};
int inner_work[] = {32, 8, 0 };

// keep it to calibrate the time of work without synchronization
#define BOX1 "baseline"
#define BOX1TEST TimeTest< TBB_Mutex<tbb::null_mutex>, SECONDS_RATIO >

// enable/disable tests for:
#define BOX2 "spin_mutex"
#define BOX2TEST TimeTest< TBB_Mutex<tbb::spin_mutex>, SECONDS_RATIO >

// enable/disable tests for:
#define BOX3 "spin_rw_mutex"
#define BOX3TEST TimeTest< TBB_Mutex<tbb::spin_rw_mutex>, SECONDS_RATIO >

// enable/disable tests for:
#define BOX4 "queuing_mutex"
#define BOX4TEST TimeTest< TBB_Mutex<tbb::queuing_mutex>, SECONDS_RATIO >

// enable/disable tests for:
//#define BOX5 "queuing_rw_mutex"
#define BOX5TEST TimeTest< TBB_Mutex<tbb::queuing_rw_mutex>, SECONDS_RATIO >

//////////////////////////////////////////////////////////////////////////////

#include <cstdlib>
#include <math.h>
#include <algorithm>                 // std::swap
#include <utility>      // Need std::pair from here
#include <sstream>
#include "tbb/tbb_stddef.h"
#include "tbb/null_mutex.h"
#include "tbb/spin_rw_mutex.h"
#include "tbb/spin_mutex.h"
#include "tbb/queuing_mutex.h"
#include "tbb/queuing_rw_mutex.h"
#include "tbb/mutex.h"

#if INTEL_TRIAL==2
#include "tbb/parallel_for.h" // enable threading by TBB scheduler
#include "tbb/task_scheduler_init.h"
#include "tbb/blocked_range.h" 
#endif
// for test
#include "time_framework.h"

using namespace tbb;
using namespace tbb::internal;

/////////////////////////////////////////////////////////////////////////////////////////

//! base class for tests family
struct TestLocks : TesterBase {
    // Inherits "value", "threads_count", and other variables
    TestLocks() : TesterBase(/*number of modes*/sizeof(outer_work)/sizeof(int)) {}
    //! returns name of test part/mode
    /*override*/std::string get_name(int testn) {
        std::ostringstream buf;
        buf.width(4); buf.fill('0');
        buf << outer_work[testn]; // mode number
        return buf.str();
    }
    //! enables results types and returns theirs suffixes
    /*override*/const char *get_result_type(int, result_t type) const {
        switch(type) {
            case MIN: return " min";
            case MAX: return " max";
            default: return 0;
        }
    }
    //! repeats count
    int repeat_until(int /*test_n*/) const {
        return REPEAT_K*100;//TODO: suggest better?
    }
    //! fake work
    void do_work(int work) volatile {
        for(int i = 0; i < work; i++) {
            volatile int x = i;
            __TBB_Pause(0); // just to call inline assembler
            x *= work/threads_count;
        }
    }
};

//! template test unit for any of TBB mutexes
template<typename M>
struct TBB_Mutex : TestLocks {
    M mutex;

    double test(int testn, int /*threadn*/)
    {
        for(int r = 0; r < repeat_until(testn); ++r) {
            do_work(outer_work[testn]);
            {
                typename M::scoped_lock with(mutex);
                do_work(/*inner work*/value);
            }
        }
        return 0;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////

//Using BOX declarations
#include "time_sandbox.h"

// run tests for each of inner work value
void RunLoops(test_sandbox &the_test, int thread) {
    for( unsigned i=0; i<sizeof(inner_work)/sizeof(int); ++i )
        the_test.factory(inner_work[i], thread);
}

int main(int argc, char* argv[]) {
    if(argc>1) Verbose = true;
    int DefThread = task_scheduler_init::default_num_threads();
    MinThread = 1; MaxThread = DefThread+1;
    ParseCommandLine( argc, argv );
    ASSERT(MinThread <= MaxThread, 0);
#if INTEL_TRIAL && defined(__TBB_parallel_for_H)
    task_scheduler_init me(MaxThread);
#endif
    {
        test_sandbox the_test("time_locked_work", StatisticsCollector::ByThreads);
        //TODO: refactor this out as RunThreads(test&)
        for( int t = MinThread; t < DefThread && t <= MaxThread; t *= 2)
            RunLoops( the_test, t ); // execute undersubscribed threads
        if( DefThread > MinThread && DefThread <= MaxThread )
            RunLoops( the_test, DefThread ); // execute on all hw threads
        if( DefThread < MaxThread)
            RunLoops( the_test, MaxThread ); // execute requested oversubscribed threads

        the_test.report.SetTitle("Time of lock/unlock for mutex Name with Outer and Inner work");
        //the_test.report.SetStatisticFormula("1AVG per size", "=AVERAGE(ROUNDS)");
        the_test.report.Print(StatisticsCollector::HTMLFile|StatisticsCollector::ExcelXML, /*ModeName*/ "Outer work");
    }
    return 0;
}

