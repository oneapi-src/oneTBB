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

#include "tbb/task.h"
#include "harness.h"
#include "tbb/task_scheduler_init.h"

using tbb::task;

#if __TBB_ipf
    const unsigned StackSize = 1024*1024*6;
#else /*  */
    const unsigned StackSize = 1024*1024*3;
#endif

// GCC and ICC on Linux store TLS data in the stack space. This test makes sure
// that the stealing limiting heuristic used by the task scheduler does not
// switch off stealing when a large amount of TLS data is reserved.
#if _MSC_VER
__declspec(thread)
#elif __linux__ || ((__MINGW32__ || __MINGW64__) && __TBB_GCC_VERSION >= 40500)
__thread
#endif
    char map2[1024*1024*2];

class TestTask : public task {
public:
    static volatile int completed;
    task* execute() {
        completed = 1;
        return NULL;
    };
};

volatile int TestTask::completed = 0;

void TestStealingIsEnabled () {
    tbb::task_scheduler_init init(2, StackSize);
    task &r = *new( task::allocate_root() ) tbb::empty_task;
    task &t = *new( r.allocate_child() ) TestTask;
    r.set_ref_count(2);
    r.spawn(t);
    int count = 0;
    while ( !TestTask::completed && ++count < 6 )
        Harness::Sleep(1000);
    ASSERT( TestTask::completed, "Stealing is disabled or the machine is heavily oversubscribed" );
    r.wait_for_all();
    task::destroy(r);
}

int TestMain () {
#if !__TBB_THREAD_LOCAL_VARIABLES_PRESENT
    REPORT( "Known issue: Test skipped because no compiler support for __thread keyword.\n" );
    return Harness::Skipped;
#endif
    if ( tbb::task_scheduler_init::default_num_threads() == 1 ) {
        REPORT( "Known issue: Test requires at least 2 hardware threads.\n" );
        return Harness::Skipped;
    }
    TestStealingIsEnabled();
    return Harness::Done;
}
