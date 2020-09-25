/*
    Copyright (c) 2020 Intel Corporation

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

#ifndef __TBB_test_common_utils_concurrency_limit_H
#define __TBB_test_common_utils_concurrency_limit_H

#include "config.h"
#include "utils_assert.h"
#include "utils_report.h"
#include "tbb/task_arena.h"

#include <cstddef>
#include <vector>
#include <algorithm>

#if _WIN32 || _WIN64
#include <windows.h>
#elif __linux__
#include <unistd.h>
#include <sys/sysinfo.h>
#include <string.h>
#include <sched.h>
#elif __FreeBSD__
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/param.h>
#include <sys/cpuset.h>
#endif
#include <thread>

namespace utils {
using thread_num_type = std::size_t;

inline thread_num_type get_platform_max_threads() {
    static thread_num_type platform_max_threads = tbb::this_task_arena::max_concurrency();
    return platform_max_threads;
}

inline std::vector<thread_num_type> concurrency_range() {
    static std::vector<thread_num_type> threads_range;
    static bool initialized = false;

    if (!initialized) {
        initialized = true;
        thread_num_type step = 1;
        for(thread_num_type thread_num = 1; thread_num <= get_platform_max_threads(); thread_num += step++)
            threads_range.push_back(thread_num);
        if(threads_range.back() != get_platform_max_threads())
            threads_range.push_back(get_platform_max_threads());
    }
    // rotate in order to make threads_range non-monotonic
    std::rotate(threads_range.begin(), threads_range.begin() + threads_range.size()/2, threads_range.end());
    return threads_range;
}

#if !__TBB_TEST_SKIP_AFFINITY

static int maxProcs = 0;

static int get_max_procs() {
    if (!maxProcs) {
#if _WIN32||_WIN64
        DWORD_PTR pam, sam, m = 1;
        GetProcessAffinityMask( GetCurrentProcess(), &pam, &sam );
        int nproc = 0;
        for ( std::size_t i = 0; i < sizeof(DWORD_PTR) * CHAR_BIT; ++i, m <<= 1 ) {
            if ( pam & m )
                ++nproc;
        }
        maxProcs = nproc;
#elif __linux__
        cpu_set_t mask;
        int result = 0;
        sched_getaffinity(0, sizeof(cpu_set_t), &mask);
        int nproc = sysconf(_SC_NPROCESSORS_ONLN);
        for (int i = 0; i < nproc; ++i) {
            if (CPU_ISSET(i, &mask)) ++result;
        }
        maxProcs = result;
#else // FreeBSD
        maxProcs = sysconf(_SC_NPROCESSORS_ONLN);
#endif
    }
    return maxProcs;
}

int get_start_affinity_process() {
#if __linux__
    cpu_set_t mask;
    sched_getaffinity(0, sizeof(cpu_set_t), &mask);

    int result = -1;

    int nproc = sysconf(_SC_NPROCESSORS_ONLN);
    for (int i = 0; i < nproc; ++i) {
        if (CPU_ISSET(i, &mask)) {
            result = i;
            break;
        }
    }
    ASSERT(result != -1, nullptr);
    return result;
#else
    // TODO: add affinity support for Windows and FreeBSD
    return 0;
#endif
}

int limit_number_of_threads( int max_threads ) {
    ASSERT(max_threads >= 1,"The limited number of threads should be positive");
    maxProcs = get_max_procs();
    if (maxProcs < max_threads) {
        // Suppose that process mask is not set so the number of available threads equals maxProcs
        return maxProcs;
    }
#if _WIN32 || _WIN64
    ASSERT(max_threads <= 64, "LimitNumberOfThreads doesn't support max_threads to be more than 64 on Windows");
    DWORD_PTR mask = 1;
    for (int i = 1; i < max_threads; ++i) {
        mask |= mask << 1;
    }
    bool err = !SetProcessAffinityMask(GetCurrentProcess(), mask);
#else
#if __linux__
    using mask_t = cpu_set_t;
#define setaffinity(mask) sched_setaffinity(getpid(), sizeof(mask_t), &mask)
#else /*__FreeBSD*/
    using mask_t = cpuset_t;
#define setaffinity(mask) cpuset_setaffinity(CPU_LEVEL_WHICH, CPU_WHICH_PID, -1, sizeof(mask_t), &mask)
#endif

    mask_t new_mask;
    CPU_ZERO(&new_mask);

    int mask_size = int(sizeof(mask_t) * CHAR_BIT);
    if ( mask_size < maxProcs ) {
        REPORT("The mask size doesn't seem to be big enough to call setaffinity. The call may return an error.");
    }

    ASSERT(max_threads <= int(sizeof(mask_t) * CHAR_BIT), "The mask size is not enough to set the requested number of threads.");
    int st = get_start_affinity_process();
    for (int i = st; i < st + max_threads; ++i) {
        CPU_SET(i, &new_mask);
    }
    int err = setaffinity(new_mask);
#endif
    ASSERT(!err, "Setting process affinity failed");
    return max_threads;
}

#endif // __TBB_TEST_SKIP_AFFINITY

} // namespace utils

#endif // __TBB_test_common_utils_concurrency_limit_H

