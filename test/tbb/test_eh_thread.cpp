/*
    Copyright (c) 2020-2021 Intel Corporation

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

#include "common/config.h"

#include "tbb/parallel_for.h"
#include "tbb/global_control.h"

#include "common/test.h"
#include "common/utils.h"
#include "common/utils_concurrency_limit.h"

#include <atomic>
#include <condition_variable>
#include <thread>
#include <vector>

//! \file test_eh_thread.cpp
//! \brief Test for [internal] functionality

// On Windows there is no real thread number limit beside available memory.
// Therefore, the test for thread limit is unreasonable.
// TODO: enable limitThreads with sanitizer under docker
#if TBB_USE_EXCEPTIONS && !_WIN32 && !__ANDROID__

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

void limitThreads(size_t limit)
{
    rlimit rlim;

    int ret = getrlimit(RLIMIT_NPROC, &rlim);
    CHECK_MESSAGE(0 == ret, "getrlimit has returned an error");

    rlim.rlim_cur = (rlim.rlim_max == (rlim_t)RLIM_INFINITY) ? limit : utils::min(limit, rlim.rlim_max);

    ret = setrlimit(RLIMIT_NPROC, &rlim);
    CHECK_MESSAGE(0 == ret, "setrlimit has returned an error");
}

static bool g_exception_caught = false;
static std::mutex m;
static std::condition_variable cv;
static std::atomic<bool> stop{ false };

static void* thread_routine(void*)
{
    std::unique_lock<std::mutex> lock(m);
    cv.wait(lock, [] { return stop == true; });
    return 0;
}

class Thread {
    pthread_t mHandle{};
    bool mValid{};
public:
    Thread() {
        mValid = false;
        pthread_attr_t attr;
        // Limit the stack size not to consume all virtual memory on 32 bit platforms.
        if (pthread_attr_init(&attr) == 0 && pthread_attr_setstacksize(&attr, 100*1024) == 0) {
            mValid = pthread_create(&mHandle, &attr, thread_routine, /* arg = */ nullptr) == 0;
        }
    }
    bool isValid() const { return mValid; }
    void join() {
        pthread_join(mHandle, nullptr);
    }
};

//! Test for exception when too many threads
//! \brief \ref resource_usage
TEST_CASE("Too many threads") {
    if (utils::get_platform_max_threads() < 2) {
        // The test expects that the scheduler will try to create at least one thread.
        return;
    }

    // Some systems set really big limit (e.g. >45К) for the number of processes/threads
    limitThreads(1024);

    std::thread /* isolate test */ ([] {
        std::vector<Thread> threads;
        stop = false;
        auto finalize = [&] {
            stop = true;
            cv.notify_all();
            for (auto& t : threads) {
                t.join();
            }
        };

        for (int i = 0;; ++i) {
            Thread thread;
            if (!thread.isValid()) {
                break;
            }
            threads.push_back(thread);
            if (i == 1024) {
                WARN_MESSAGE(false, "setrlimit seems having no effect");
                finalize();
                return;
            }
        }
        g_exception_caught = false;
        try {
            // Initialize the library to create worker threads
            tbb::parallel_for(0, 2, [](int) {});
        } catch (const std::exception & e) {
            g_exception_caught = true;
            // Do not CHECK to avoid memory allocation (we can be out of memory)
            if (e.what()== nullptr) {
                FAIL("Exception does not have description");
            }
        }
        // Do not CHECK to avoid memory allocation (we can be out of memory)
        if (!g_exception_caught) {
            FAIL("No exception was caught");
        }
        finalize();
    }).join();
}
#endif
