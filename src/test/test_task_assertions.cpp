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

// Test correctness of forceful TBB initialization before any dynamic initialization
// of static objects inside the library took place.
namespace tbb { 
namespace internal {
    // Forward declaration of the TBB general initialization routine from task.cpp
    void DoOneTimeInitializations();
}}

struct StaticInitializationChecker {
    StaticInitializationChecker () { tbb::internal::DoOneTimeInitializations(); }
} theChecker;

//------------------------------------------------------------------------
// Test that important assertions in class task fail as expected.
//------------------------------------------------------------------------

#define HARNESS_NO_PARSE_COMMAND_LINE 1
#include "harness_inject_scheduler.h"
#include "harness.h"
#include "harness_bad_expr.h"

#if TRY_BAD_EXPR_ENABLED
//! Task that will be abused.
tbb::task* volatile AbusedTask;

//! Number of times that AbuseOneTask
int AbuseOneTaskRan;

//! Body used to create task in thread 0 and abuse it in thread 1.
struct AbuseOneTask {
    void operator()( int ) const {
        tbb::task_scheduler_init init;
        // Thread 1 attempts to incorrectly use the task created by thread 0.
        tbb::task_list list;
        // spawn_root_and_wait over empty list should vacuously succeed.
        tbb::task::spawn_root_and_wait(list);

        // Check that spawn_root_and_wait fails on non-empty list. 
        list.push_back(*AbusedTask);

        // Try abusing recycle_as_continuation
        TRY_BAD_EXPR(AbusedTask->recycle_as_continuation(), "execute" );
        TRY_BAD_EXPR(AbusedTask->recycle_as_safe_continuation(), "execute" );
        TRY_BAD_EXPR(AbusedTask->recycle_to_reexecute(), "execute" );
        ++AbuseOneTaskRan;
    }
};

//! Test various __TBB_ASSERT assertions related to class tbb::task.
void TestTaskAssertions() {
    // Catch assertion failures
    tbb::set_assertion_handler( AssertionFailureHandler );
    tbb::task_scheduler_init init;
    // Create task to be abused
    AbusedTask = new( tbb::task::allocate_root() ) tbb::empty_task;
    NativeParallelFor( 1, AbuseOneTask() );
    ASSERT( AbuseOneTaskRan==1, NULL );
    tbb::task::destroy(*AbusedTask);
    // Restore normal assertion handling
    tbb::set_assertion_handler( ReportError );
}

int TestMain () {
    TestTaskAssertions();
    return Harness::Done;
}

#else /* !TRY_BAD_EXPR_ENABLED */

int TestMain () {
    return Harness::Skipped;
}

#endif /* !TRY_BAD_EXPR_ENABLED */
