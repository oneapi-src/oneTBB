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

// Used in tests that work with TBB scheduler but do not link to the TBB library.
// In other words it embeds the TBB library core into the test executable.

#ifndef harness_inject_scheduler_H
#define harness_inject_scheduler_H

#if HARNESS_DEFINE_PRIVATE_PUBLIC
#include <string> // merely prevents LNK2019 error to happen (on ICL+VC9 configurations)
#include <algorithm> // include it first to avoid error on define below
#define private public
#define protected public
#endif

// Suppress usage of #pragma comment
#define __TBB_NO_IMPLICIT_LINKAGE 1

// Enable preview features if any 
#define __TBB_BUILD 1

#undef DO_ITT_NOTIFY

#define __TBB_SOURCE_DIRECTLY_INCLUDED 1
#include "../tbb/tbb_main.cpp"
#include "../tbb/dynamic_link.cpp"
#include "../tbb/tbb_misc_ex.cpp"

// Tasking subsystem files
#include "../tbb/governor.cpp"
#include "../tbb/market.cpp"
#include "../tbb/arena.cpp"
#include "../tbb/scheduler.cpp"
#include "../tbb/observer_proxy.cpp"
#include "../tbb/task.cpp"
#include "../tbb/task_group_context.cpp"

// Other dependencies
#include "../tbb/cache_aligned_allocator.cpp"
#include "../tbb/tbb_thread.cpp"
#include "../tbb/mutex.cpp"
#include "../tbb/spin_rw_mutex.cpp"
#include "../tbb/spin_mutex.cpp"
#include "../tbb/private_server.cpp"
#include "../tbb/concurrent_monitor.cpp"
#if _WIN32||_WIN64
#include "../tbb/semaphore.cpp"
#endif
#include "../rml/client/rml_tbb.cpp"

#if HARNESS_USE_RUNTIME_LOADER
#undef HARNESS_USE_RUNTIME_LOADER
#include "harness.h"

int TestMain () {
    // Tests that directly include sources make no sense in runtime loader testing mode.
    return Harness::Skipped;
}
// Renaming the TestMain function avoids conditional compilation around same function in the test file
#define TestMain TestMainSkipped
#endif

#if HARNESS_DEFINE_PRIVATE_PUBLIC
#undef protected
#undef private
#endif

#endif /* harness_inject_scheduler_H */
