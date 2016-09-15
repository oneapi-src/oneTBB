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

#if _USRDLL

#include <stdlib.h>
#include "harness_defs.h"
#include "tbb/scalable_allocator.h"
#if __TBB_SOURCE_DIRECTLY_INCLUDED
#include "../tbbmalloc/tbbmalloc_internal_api.h"
#endif

#define HARNESS_CUSTOM_MAIN 1
#define HARNESS_NO_PARSE_COMMAND_LINE 1
#include "harness.h"
#include "harness_assert.h"

#if _WIN32||_WIN64
extern "C" {
    extern __declspec(dllexport) void callDll();
}
#endif

extern "C" void callDll()
{
    static const int NUM = 20;
    void *ptrs[NUM];

    for (int i=0; i<NUM; i++) {
        ptrs[i] = scalable_malloc(i*1024);
        ASSERT(ptrs[i], NULL);
    }
    for (int i=0; i<NUM; i++)
        scalable_free(ptrs[i]);
}

#if __TBB_SOURCE_DIRECTLY_INCLUDED

struct RegisterProcessShutdownNotification {
    ~RegisterProcessShutdownNotification() {
        __TBB_mallocProcessShutdownNotification();
    }
};

static RegisterProcessShutdownNotification reg;

#endif

#else // _USRDLL

#define __TBB_NO_IMPLICIT_LINKAGE 1
#include "harness_dynamic_libs.h"
#if __TBB_WIN8UI_SUPPORT
// FIXME: fix the test to support Windows* 8 Store Apps mode.
#define HARNESS_SKIP_TEST 1
#endif
#define HARNESS_NO_PARSE_COMMAND_LINE 1
#include "harness.h"

#if !HARNESS_SKIP_TEST

#include "harness_memory.h"
#include "harness_tbb_independence.h"
#include "harness_barrier.h"

class UseDll {
    Harness::FunctionAddress run;
public:
    UseDll(Harness::FunctionAddress runPtr) : run(runPtr) { }
    void operator()( int /*id*/ ) const {
        (*run)();
    }
};

void LoadThreadsUnload()
{
    Harness::LIBRARY_HANDLE lib =
        Harness::OpenLibrary(TEST_LIBRARY_NAME("test_malloc_used_by_lib_dll"));
    ASSERT(lib, "Can't load " TEST_LIBRARY_NAME("test_malloc_used_by_lib_dll"));
    NativeParallelFor( 4, UseDll( Harness::GetAddress(lib, "callDll") ) );
    Harness::CloseLibrary(lib);
}

struct UnloadCallback {
    Harness::LIBRARY_HANDLE lib;

    void operator() () const {
        Harness::CloseLibrary(lib);
    }
};

struct RunWithLoad : NoAssign {
    static Harness::SpinBarrier startBarr, endBarr;
    static UnloadCallback unloadCallback;
    static Harness::FunctionAddress runPtr;

    void operator()(int id) const {
        if (!id) {
            Harness::LIBRARY_HANDLE lib =
                Harness::OpenLibrary(TEST_LIBRARY_NAME("test_malloc_used_by_lib_dll"));
            ASSERT(lib, "Can't load " TEST_LIBRARY_NAME("test_malloc_used_by_lib_dll"));
            runPtr = Harness::GetAddress(lib, "callDll");
            unloadCallback.lib = lib;
        }
        startBarr.wait();
        (*runPtr)();
        endBarr.wait(unloadCallback);
    }
};

Harness::SpinBarrier RunWithLoad::startBarr, RunWithLoad::endBarr;
UnloadCallback RunWithLoad::unloadCallback;
Harness::FunctionAddress RunWithLoad::runPtr;

void ThreadsLoadUnload()
{
    const int threads = 4;

    RunWithLoad::startBarr.initialize(threads);
    RunWithLoad::endBarr.initialize(threads);
    NativeParallelFor(threads, RunWithLoad());
}

int TestMain () {
    const int ITERS = 20;
    int i;
    std::ptrdiff_t memory_leak = 0;

    GetMemoryUsage();

    for (int run = 0; run<2; run++) {
        // expect that memory consumption stabilized after several runs
        for (i=0; i<ITERS; i++) {
            std::size_t memory_in_use = GetMemoryUsage();
            if (run)
                LoadThreadsUnload();
            else
                ThreadsLoadUnload();
            memory_leak = GetMemoryUsage() - memory_in_use;
            if (memory_leak == 0)  // possibly too strong?
                break;
        }
        if(i==ITERS) {
            // not stabilized, could be leak
            REPORT( "Error: memory leak of up to %ld bytes\n", static_cast<long>(memory_leak));
            exit(1);
        }
    }

    return Harness::Done;
}

#endif /* HARNESS_SKIP_TEST */
#endif // _USRDLL
