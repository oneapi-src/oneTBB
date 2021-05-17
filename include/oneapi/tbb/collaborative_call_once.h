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

#ifndef __TBB_collaborative_call_once_H
#define __TBB_collaborative_call_once_H

#include "tbb/task_arena.h"
#include "tbb/task_group.h"
#include "tbb/task.h"

#include <atomic>

namespace tbb {
namespace detail {
namespace d1 {

#if _MSC_VER && !defined(__INTEL_COMPILER)
    // Suppress warning: structure was padded due to alignment specifier
    #pragma warning (push)
    #pragma warning (disable: 4324)
#endif

constexpr std::size_t bit_count = 7;

static_assert(1 << bit_count == max_nfs_size, "bit_count must be a log2(max_nfs_size)");

inline constexpr std::uintptr_t maskoff_pointer(std::uintptr_t ptr) {
    return ptr & (std::size_t(-1) << bit_count);
}

class alignas(max_nfs_size) run_once {
    task_arena my_arena;
    wait_context my_wait_context;
    task_group_context my_context;

    std::atomic<std::int64_t> ref_count{0};

    template <typename Func>
    class call_once_delegate : public delegate_base {
    public:
        call_once_delegate(Func& f) : my_func(f) {}

        bool operator()() const override {
            my_func();
            return true;
        }

    private:
        Func& my_func;
    };

    template<typename Fn>
    void isolated_execute(Fn f) {
        call_once_delegate<Fn> delegate(f);

        r1::isolate_within_arena(delegate, reinterpret_cast<std::intptr_t>(this));
    }

public:
    run_once()
        : my_arena(task_arena::attach{})
        , my_wait_context(0)
        , my_context(task_group_context::bound,
            task_group_context::default_traits | task_group_context::concurrent_wait)
        {}

    ~run_once() {
        spin_wait_while(ref_count, [&](std::int64_t value) { return value > 0; }, std::memory_order_acquire);
    }

    void increase_ref() { ref_count++; }

    void decrease_ref() { ref_count--; }

    template <typename F>
    void run_and_wait(F&& f) {
        my_arena.execute([&] {
            isolated_execute([&] {
                function_stack_task<F> t{ std::forward<F>(f), my_wait_context };
                my_wait_context.reserve();
                
                execute_and_wait(t, my_context, my_wait_context, my_context);
            });
        });
    }

    void wait() {
        my_arena.execute([&] {
            isolated_execute([&] {
                // We do not want to get an exception from user functor on moonlighting threads.
                // The exception is handled with the winner thread
                task_group_context stub_context;
                detail::d1::wait(my_wait_context, stub_context);
            });
        });
    }

};

class collaborative_once_flag : no_copy {
    enum state {uninitialized, done};
    std::atomic<std::uintptr_t> my_state{ state::uninitialized };

    template <typename Fn, typename... Args>
    friend void collaborative_call_once(collaborative_once_flag& flag, Fn&& f, Args&&... args);

    template <typename Fn>
    void do_collaborative_call_once(Fn&& f) {
        std::uintptr_t expected = my_state.load(std::memory_order_acquire);
        run_once local_runner;
        while (expected != state::done) {
            if (expected == state::uninitialized && my_state.compare_exchange_strong(expected, reinterpret_cast<std::uintptr_t>(&local_runner))) {
                // winner
                auto local_expected = reinterpret_cast<std::uintptr_t>(&local_runner);
                try_call([&] {
                    local_runner.run_and_wait(std::forward<Fn>(f));
                }).on_exception([&] {
                    while (!my_state.compare_exchange_strong(local_expected, state::uninitialized, std::memory_order_release, std::memory_order_relaxed)) {
                        local_expected = reinterpret_cast<std::uintptr_t>(&local_runner);
                    }
                });

                while (!my_state.compare_exchange_strong(local_expected, state::done, std::memory_order_release, std::memory_order_relaxed)) {
                    local_expected = reinterpret_cast<std::uintptr_t>(&local_runner);
                }
                return;
            } else {
                // moonlighting thread
                do {
                    auto max_value = expected | (max_nfs_size-1);
                    expected = spin_wait_while_eq(my_state, max_value);
                // "expected > state::done" prevents storing values, when state is uninitialized
                } while (expected > state::done && !my_state.compare_exchange_strong(expected, expected + 1));

                if (auto runner = reinterpret_cast<run_once*>(maskoff_pointer(expected))) {

                    runner->increase_ref();
                    my_state.fetch_sub(1);

                    // The moonlighting threads are not expected to handle exceptions from user functor.
                    // Therefore, no exception is expected from wait().
                    [runner] () noexcept { runner->wait(); }();

                    runner->decrease_ref();
                }
            }
        }
    }
};


template <typename Fn, typename... Args>
void collaborative_call_once(collaborative_once_flag& flag, Fn&& fn, Args&&... args) {
    auto func = [&] { fn(std::forward<Args>(args)...); };
    flag.do_collaborative_call_once(func);
}

#if _MSC_VER && !defined(__INTEL_COMPILER)
    #pragma warning (pop) // 4324 warning
#endif

} // namespace d1
} // namespace detail

using detail::d1::collaborative_call_once;
using detail::d1::collaborative_once_flag;
} // namespace tbb

#endif // __TBB_collaborative_call_once_H
