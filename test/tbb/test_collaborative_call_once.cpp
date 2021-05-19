/*
    Copyright (c) 2021 Intel Corporation

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

#define TBB_PREVIEW_COLLABORATIVE_CALL_ONCE 1
#if _MSC_VER && !defined(__INTEL_COMPILER)
// unreachable code
#pragma warning( push )
#pragma warning( disable: 4702 )
#endif

#include "common/test.h"
#include "common/exception_handling.h"
#include "common/utils_concurrency_limit.h"


#include "tbb/collaborative_call_once.h"
#include "tbb/parallel_invoke.h"
#include "tbb/parallel_reduce.h"
#include "tbb/task_arena.h"

#include <atomic>
#include <mutex>


struct increment_functor {
    std::atomic<int> ct{0};

    void operator()() {
        ct++;
    }
};

struct sum_functor {
    int sum{0};

    template<typename T>
    void operator()(T first_op) {
        sum += first_op;
    }

    template<typename T, typename... Args>
    void operator()(T first_op, Args&&... args) {
        (*this)(first_op);
        (*this)(std::forward<Args>(args)...);
    }
};

class call_once_exception : public std::exception {};

template<typename Fn, typename... Args>
void call_once_in_for_loop(Fn& body, Args&&... args) {
    tbb::collaborative_once_flag flag;
    for (int i = 0; i < 1024; ++i) {
        tbb::collaborative_call_once(flag, body, std::forward<Args>(args)...);
    }
}

template<typename Fn, typename... Args>
void call_once_in_parallel_for(Fn& body, Args&&... args) {
    tbb::collaborative_once_flag flag;
#if __GNUC__ && !__TBB_GCC_PARAMETER_PACK_IN_LAMBDAS_PRESENT
    auto stored_pack = tbb::detail::d0::save_pack(std::forward<Args>(args)...);
    auto func = [&] { tbb::detail::d0::call(body, stored_pack); };
#endif // !__TBB_GCC_PARAMETER_PACK_IN_LAMBDAS_PRESENT

    tbb::parallel_for(tbb::blocked_range<size_t>(0, 100), [&](const tbb::blocked_range<size_t>& range) {
        for (size_t i = range.begin(); i != range.end(); ++i) {
#if __GNUC__ && !__TBB_GCC_PARAMETER_PACK_IN_LAMBDAS_PRESENT
            tbb::collaborative_call_once(flag, func);
#else
            tbb::collaborative_call_once(flag, body, std::forward<Args>(args)...);
#endif // __TBB_GCC_PARAMETER_PACK_IN_LAMBDAS_PRESENT
        }
    });
}

template<typename Fn, typename... Args>
void call_once_threads(Fn& body, Args&&... args) {
    tbb::collaborative_once_flag flag;
    std::vector<std::thread> threads;

#if __GNUC__ && !__TBB_GCC_PARAMETER_PACK_IN_LAMBDAS_PRESENT
    auto stored_pack = tbb::detail::d0::save_pack(std::forward<Args>(args)...);
    auto func = [&] { tbb::detail::d0::call(body, stored_pack); };
#endif // !__TBB_GCC_PARAMETER_PACK_IN_LAMBDAS_PRESENT

    for (int i = 0; i < 100; ++i)
    {
        threads.push_back(std::thread([&]() {
#if __GNUC__ && !__TBB_GCC_PARAMETER_PACK_IN_LAMBDAS_PRESENT
            tbb::collaborative_call_once(flag, func);
#else
            tbb::collaborative_call_once(flag, body, std::forward<Args>(args)...);
#endif // __TBB_GCC_PARAMETER_PACK_IN_LAMBDAS_PRESENT
        }));
    }
    for (auto& thread : threads) {
        thread.join();
    }
}

TEST_CASE("only calls once 1") {
    {
        increment_functor f;

        call_once_in_for_loop(f);

        REQUIRE( f.ct == 1);
    }
    {
        increment_functor f;

        call_once_in_parallel_for(f);

        REQUIRE(f.ct == 1);
    }
    {
        increment_functor f;

        call_once_threads(f);

        REQUIRE(f.ct == 1);
    }
}

TEST_CASE("only calls once 2") {
    {
        sum_functor f;

        call_once_in_for_loop(f, 1, 2, 3 ,4);

        REQUIRE(f.sum == 10);
    }
    {
        sum_functor f;

        call_once_in_parallel_for(f, 1000, -1000);

        REQUIRE(f.sum == 0);
    }
    {
        sum_functor f;

        call_once_threads(f, 0, -1, -5);

        REQUIRE(f.sum == -6);
    }
}

TEST_CASE("only calls once - stress test") {
    {
        increment_functor f;

        tbb::collaborative_once_flag flag;
        std::vector<std::thread> threads;
        for (std::size_t i = 0; i < utils::get_platform_max_threads()*5; ++i)
        {
            threads.push_back(std::thread([&]() {
                tbb::collaborative_call_once(flag, f);
            }));
        }
        for (auto& thread : threads) {
            thread.join();
        }
        REQUIRE(f.ct == 1);
    }
    {
        increment_functor f;

        utils::SpinBarrier barrier(utils::get_platform_max_threads()*5);
        tbb::collaborative_once_flag flag;
        utils::NativeParallelFor(utils::get_platform_max_threads()*5, [&](std::size_t) {
            for (int i = 0; i < 100; ++i) {
                REQUIRE(f.ct == i);
                barrier.wait([&] {
                    flag.~collaborative_once_flag();
                    new (&flag) tbb::collaborative_once_flag{};
                });
                tbb::collaborative_call_once(flag, f);
            }
        });
    }
}

#if TBB_USE_EXCEPTIONS
TEST_CASE("handles exceptions - state reset") {

    std::atomic<bool> b{ false };
    auto setB = [&b]() { b = true; };
    auto setBAndCancel = [&b]() {
        b = true;
        throw call_once_exception{};
    };

    tbb::collaborative_once_flag flag;
    REQUIRE_THROWS_AS(tbb::collaborative_call_once(flag, setBAndCancel), call_once_exception);
    REQUIRE(b);

    b = false;
    REQUIRE_THROWS_AS(tbb::collaborative_call_once(flag, setBAndCancel), call_once_exception);
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

TEST_CASE("handles exceptions - stress test") {
    std::atomic<int> data{0};

    bool run_again{true};

    auto throwing_func = [&] {
        utils::doDummyWork(10000);
        if (data < 100) {
            data++;
            run_again = true;
            throw call_once_exception{};
        }
        run_again = false;
    };

    tbb::collaborative_once_flag flag;

    utils::NativeParallelFor(utils::get_platform_max_threads()*5, [&](std::size_t) {
        while(run_again) {
            try {
                tbb::collaborative_call_once(flag, throwing_func);
            } catch(...) {
            }
        }
    });
    REQUIRE(data == 100);
}

#endif

TEST_CASE("multiple help") {
    // Test multiple threads do the work:
    std::mutex mutex;
    std::map<int, int> id_ct_map;
    std::atomic<int> call_ct{0};

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
        tbb::parallel_for(tbb::blocked_range<size_t>(0, num_work_pieces, 1), [&](const tbb::blocked_range<size_t>& range) {
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
        return std::count_if(cts.begin(), cts.end(), [](const std::pair<int, int>& it) { return it.second >= 2; }) >= tbb::this_task_arena::max_concurrency() / 3;
    };
    {
        id_ct_map.clear();
        call_ct = 0;

        call_once_threads(work);

        REQUIRE(call_ct == 1);
        REQUIRE(isParallel(id_ct_map));
    }
    {
        id_ct_map.clear();
        call_ct = 0;

        call_once_in_parallel_for(work);

        REQUIRE(call_ct == 1);
        REQUIRE(isParallel(id_ct_map));
    }
}

std::uint64_t naive_recursive_fib(int n) {
    if (n <= 1) {
        return 1;
    }
    std::uint64_t a, b;
    tbb::parallel_invoke([&] { a = naive_recursive_fib(n - 2); },
                         [&] { b = naive_recursive_fib(n - 1); });
    return a + b;
}

using FibBuffer = std::vector<std::pair<tbb::collaborative_once_flag, std::uint64_t>>;
std::uint64_t collaborative_recursive_fib(int n, FibBuffer& buffer) {
    if (n <= 1) {
        return 1;
    }
    tbb::collaborative_call_once(buffer[n].first, [&]() {
        std::uint64_t a, b;
        tbb::parallel_invoke([&] { a = collaborative_recursive_fib(n - 2, buffer); },
                             [&] { b = collaborative_recursive_fib(n - 1, buffer); });
        buffer[n].second = a + b;
    });
    return buffer[n].second;
}

std::uint64_t collaborative_recursive_fib(int n) {
    FibBuffer buffer(n+1);
    return collaborative_recursive_fib(n, buffer);
}

TEST_CASE("fibonacci example") {
    constexpr int N = 25;

    auto naive = naive_recursive_fib(N);

    auto collaborative = collaborative_recursive_fib(N);

    REQUIRE(naive == collaborative);
}

#if _MSC_VER && !defined(__INTEL_COMPILER)
#pragma warning( pop )
#endif
