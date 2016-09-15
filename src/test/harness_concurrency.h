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

#ifndef tbb_tests_harness_concurrency_H
#define tbb_tests_harness_concurrency_H

#if _WIN32||_WIN64
#include "tbb/machine/windows_api.h"
#elif __linux__
#include <unistd.h>
#include <sys/sysinfo.h>
#include <string.h>
#include <sched.h>
#elif __FreeBSD__
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/param.h>  // Required by <sys/cpuset.h>
#include <sys/cpuset.h>
#endif

#include <limits.h>

namespace Harness {
    static int maxProcs = 0;
    static int GetMaxProcs() {
        if ( !maxProcs ) {
#if _WIN32||_WIN64
            SYSTEM_INFO si;
            GetNativeSystemInfo(&si);
            maxProcs = si.dwNumberOfProcessors;
#elif __linux__
            maxProcs = get_nprocs();
#else /* __FreeBSD__ */
            maxProcs = sysconf(_SC_NPROCESSORS_ONLN);
#endif
        }
        return maxProcs;
    }

    int LimitNumberOfThreads(int max_threads) {
        ASSERT( max_threads >= 1 , "The limited number of threads should be positive." );
        maxProcs = GetMaxProcs();
        if ( maxProcs < max_threads )
            // Suppose that process mask is not set so the number of available threads equals maxProcs
            return maxProcs;

#if _WIN32||_WIN64
        ASSERT( max_threads <= 64 , "LimitNumberOfThreads doesn't support max_threads to be more than 64 on Windows." ); 
        DWORD_PTR mask = 1;
        for ( int i = 1; i < max_threads; ++i )
            mask |= mask << 1;
        bool err = !SetProcessAffinityMask( GetCurrentProcess(), mask );
#else /* !WIN */
#if __linux__
        typedef cpu_set_t mask_t;
#if __TBB_MAIN_THREAD_AFFINITY_BROKEN
#define setaffinity(mask) sched_setaffinity(0 /*get the mask of the calling thread*/, sizeof(mask_t), &mask)
#else
#define setaffinity(mask) sched_setaffinity(getpid(), sizeof(mask_t), &mask)
#endif
#else /* __FreeBSD__ */
        typedef cpuset_t mask_t;
#if __TBB_MAIN_THREAD_AFFINITY_BROKEN
#define setaffinity(mask) cpuset_setaffinity(CPU_LEVEL_WHICH, CPU_WHICH_TID, -1, sizeof(mask_t), &mask)
#else
#define setaffinity(mask) cpuset_setaffinity(CPU_LEVEL_WHICH, CPU_WHICH_PID, -1, sizeof(mask_t), &mask)
#endif
#endif /* __FreeBSD__ */
        mask_t newMask;
        CPU_ZERO(&newMask);

        int maskSize = (int)sizeof(mask_t) * CHAR_BIT;
        ASSERT_WARNING( maskSize >= maxProcs, "The mask size doesn't seem to be big enough to call setaffinity. The call may return an error." );

        ASSERT( max_threads <= (int)sizeof(mask_t) * CHAR_BIT , "The mask size is not enough to set the requested number of threads." );
        for ( int i = 0; i < max_threads; ++i )
            CPU_SET( i, &newMask );
        int err = setaffinity( newMask );
#endif /* !WIN */
        ASSERT( !err, "Setting process affinity failed" );

        return max_threads;
    }

} // namespace Harness

#endif /* tbb_tests_harness_concurrency_H */
