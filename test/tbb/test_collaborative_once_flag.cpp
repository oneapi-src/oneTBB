/*
    Copyright (c) 2005-2021 Intel Corporation

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

#include "common/test.h"
#include "common/exception_handling.h"

#include "tbb/collaborative_once_flag.h"
#include "tbb/parallel_invoke.h"
#include "tbb/parallel_reduce.h"
#include "tbb/task_arena.h"

#include <atomic>
#include <mutex>

static_assert(sizeof(tbb::collaborative_once_flag) <= 24, "Make sure you intened to increase the size of collaborative_once_flag");

TEST_CASE("Сollaborative_once_flag - only calls once") {
    std::atomic<int> ct = 0;
    auto f = [&]() {
        ct++;
    };

    {
        ct = 0;
        tbb::collaborative_once_flag flag;
        for (int i = 0; i != 100; ++i) {
            tbb::collaborative_call_once(flag, f);
        }
        CHECK(ct == 1);
    }
    {
        ct = 0;
        tbb::collaborative_once_flag flag;
        tbb::parallel_for(tbb::blocked_range<size_t>(0, 100), [&](const auto& range) {
            for (int i = range.begin(); i != range.end(); ++i) {
                tbb::collaborative_call_once(flag, f);
            }
        });
        CHECK(ct == 1);
    }
    {
        ct = 0;
        tbb::collaborative_once_flag flag;
        std::vector<std::thread> threads;  // C++20: std::jthread
        for (int i = 0; i != 100; ++i)
        {
            threads.push_back(std::thread([&]() {
                tbb::collaborative_call_once(flag, f);
            }));
        }
        for (auto& thread : threads) {
            thread.join();
        }
        CHECK(ct == 1);
    }
}

TEST_CASE("Сcollaborative_once_flag - handles exceptions correctly") {
    // Try to confuse call_once by canceling out from under it:
    class LocalException : public std::exception {};

    std::atomic<bool> b{ false };
    auto setB = [&b]() { b = true; };
    auto setBAndCancel = [&b]() {
        b = true;
        throw LocalException();
    };
    tbb::collaborative_once_flag flag;
    REQUIRE_THROWS_AS(tbb::collaborative_call_once(flag, setBAndCancel), LocalException);
    REQUIRE(b);
    b = false;
    REQUIRE_THROWS_AS(tbb::collaborative_call_once(flag, setBAndCancel), LocalException);
    REQUIRE(b);
    b = false;
    tbb::collaborative_call_once(flag, setB);
    REQUIRE(b);
    b = false;
    tbb::collaborative_call_once(flag, setB); // Now the call_once flag should be set.
    REQUIRE(!b);
    b = false;
    REQUIRE_NOTHROW(tbb::collaborative_call_once(flag, setBAndCancel)); // Flag still set, so it shouldn't be called.
    REQUIRE(!b);
}

TEST_CASE("collaborative_once_flag - multiple help") {
    // Test mutliple threads do the work:
    std::mutex mutex;
    std::map<int, int> id_ct_map;
    std::atomic<int> call_ct = 0;

    const size_t num_work_pieces = tbb::this_task_arena::max_concurrency() * 512;
    const auto time_per_work_piece = std::chrono::microseconds(5);
    // this allows us to make sure all the calls in the outer loop get stuck (if they will) before the inner loop begins
    const auto time_initial_delay = std::chrono::milliseconds(100);

    auto busy_wait = [](std::chrono::microseconds time) {
        auto start_t = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() < start_t + time) {
            // busy wait
        }
    };
    auto work = [&]() {
        call_ct++;
        busy_wait(time_initial_delay);
        tbb::parallel_for(tbb::blocked_range<size_t>(0, num_work_pieces, 1), [&](const auto& range) {
            for (size_t i = range.begin(); i != range.end(); ++i) {
                busy_wait(time_per_work_piece);
            }
            std::lock_guard<std::mutex> lock(mutex);
            int id = tbb::detail::d1::current_thread_index();
            id_ct_map[id] += range.size();
        });
    };
    // check that a bunch of the threads did some of the work
    auto isParallel = [](const std::map<int, int> cts) {
        return std::count_if(cts.begin(), cts.end(), [](const auto& it) { return it.second >= 2; }) >= tbb::this_task_arena::max_concurrency() / 3;
    };
    {
        // std::call_once from jthreads
        id_ct_map.clear();
        call_ct = 0;
        std::once_flag flag;
        std::vector<std::thread> threads; // C++20: std::jthread
        for (int i = 0; i < 1024; i++) {
            threads.push_back(std::thread([&]() { std::call_once(flag, work); }));
        }
        for (auto& t : threads) {
            t.join();
        }
        CHECK(call_ct == 1);
        CHECK(isParallel(id_ct_map));
    }
    {
        // tbb::collaborative_once_flag from jthreads
        id_ct_map.clear();
        call_ct = 0;
        tbb::collaborative_once_flag flag;
        std::vector<std::thread> threads; // C++20: std::jthread
        for (int i = 0; i < 1024; i++) {
            threads.push_back(std::thread([&]() { tbb::collaborative_call_once(flag, work); }));
        }
        for (auto& t : threads) {
            t.join();
        }
        CHECK(call_ct == 1);
        CHECK(isParallel(id_ct_map));
    }
    {
        // std::once_flag from tbb::parallel_for (This one would occasionally deadlock without an explicit call to isolate)
        id_ct_map.clear();
        call_ct = 0;
        std::once_flag flag;
        tbb::parallel_for(tbb::blocked_range<size_t>(0, 64, 1), [&](const auto& range) {
            for (size_t i = range.begin(); i != range.end(); ++i) {
                std::call_once(flag, [&]() {
                    // Without this explicit call to isolate, this code sometimes deadlocks
                    tbb::this_task_arena::isolate(work);
                });
            }
        });
        CHECK(call_ct == 1);
        CHECK(!isParallel(id_ct_map));
    }
    {
        // tbb::collaborative_once_flag from tbb::parallel_for
        id_ct_map.clear();
        call_ct = 0;
        tbb::collaborative_once_flag flag;
        tbb::parallel_for(tbb::blocked_range<size_t>(0, 64, 1), [&](const auto& range) {
            for (size_t i = range.begin(); i != range.end(); ++i) {
                tbb::collaborative_call_once(flag, work);
            }
        });
        CHECK(call_ct == 1);
        CHECK(isParallel(id_ct_map));
    }
};

int64_t naive_recursive_fib(int n) {
    if (n <= 1) {
        return 1;
    }
    int64_t a, b;
    tbb::parallel_invoke([&] { a = naive_recursive_fib(n - 2); },
                         [&] { b = naive_recursive_fib(n - 1); });
    return a + b;
}

using FibBuffer = std::vector<std::pair<tbb::collaborative_once_flag, uint64_t>>;
int64_t collaborative_recursive_fib(int n, FibBuffer& buffer) {
    if (n <= 1) {
        return 1;
    }
    tbb::collaborative_call_once(buffer[n].first, [&]() {
        int64_t a, b;
        tbb::parallel_invoke([&] { a = collaborative_recursive_fib(n - 2, buffer); },
                             [&] { b = collaborative_recursive_fib(n - 1, buffer); });
        buffer[n].second = a + b;
    });
    return buffer[n].second;
}

int64_t collaborative_recursive_fib(int n) {
    FibBuffer buffer(n+1);
    return collaborative_recursive_fib(n, buffer);
}

TEST_CASE("collaborative_once_flag - fib example") {
    using namespace std::chrono;

    // For N = 25, I get the following
    // naive takes 70.78 ms
    // collaborative takes 0.41 ms

    const int N = 25;

    auto start_t = steady_clock::now();
    auto naive = naive_recursive_fib(N);
    auto naive_time = steady_clock::now() - start_t;

    start_t = steady_clock::now();
    auto collaborative = collaborative_recursive_fib(N);
    auto collaborative_time = steady_clock::now() - start_t;

    CHECK(naive == collaborative);
    CHECK(naive_time > collaborative_time * 25);
}
