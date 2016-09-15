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

#include "harness_defs.h"

#if __TBB_TEST_SKIP_AFFINITY
#define HARNESS_NO_PARSE_COMMAND_LINE 1
#include "harness.h"
int TestMain() {
    return Harness::Skipped;
}
#else /* affinity mask can be set and used by TBB */

#include "harness.h"
#include "harness_concurrency.h"

#include "tbb/task_scheduler_init.h"
#include "tbb/tbb_thread.h"
#include "tbb/enumerable_thread_specific.h"

// The declaration of a global ETS object is needed to check that
// it does not initialize the task scheduler, and in particular
// does not set the default thread number. TODO: add other objects
// that should not initialize the scheduler.
tbb::enumerable_thread_specific<std::size_t> ets;

int TestMain () {
    int maxProcs = Harness::GetMaxProcs();

    if ( maxProcs < 2 )
        return Harness::Skipped;

    int availableProcs = maxProcs/2;
    ASSERT( Harness::LimitNumberOfThreads( availableProcs ) == availableProcs, "LimitNumberOfThreads has not set the requested limitation." );
    ASSERT( tbb::task_scheduler_init::default_num_threads() == availableProcs, NULL );
    ASSERT( (int)tbb::tbb_thread::hardware_concurrency() == availableProcs, NULL );
    return Harness::Done;
}
#endif /* __TBB_TEST_SKIP_AFFINITY */
