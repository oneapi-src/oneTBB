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

#if __TBB_PREVIEW_COLLABORATIVE_CALL_ONCE

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

class alignas(max_nfs_size) once_runner {

    struct storage_t {
        task_arena m_arena{ task_arena::attach{} };
        wait_context m_wait_context{1};
    };

    template <typename Func>
    class call_once_delegate : public delegate_base {
    public:
        call_once_delegate(Func& f) : m_func(f) {}

        bool operator()() const override {
            m_func();
            return true;
        }

    private:
        Func& m_func;
    };

    class lifetime_tracker : no_copy {
        once_runner& m_runner;
    public:
    lifetime_tracker(once_runner& r) : m_runner(r) {
        m_runner.increase_ref();
    }
    ~lifetime_tracker() {
        m_runner.decrease_ref();
    }
};

    std::atomic<std::int64_t> ref_count{0};
    std::atomic<bool> is_ready{false};

    // Storage with task_arena and contexts must be initialized only by winner thread
    // by calling make_runner() method
    union {
        storage_t m_storage;
    };

    template<typename Fn>
    void isolated_execute(Fn f) {
        call_once_delegate<Fn> delegate(f);

        r1::isolate_within_arena(delegate, reinterpret_cast<std::intptr_t>(this));
    }

    void increase_ref() { ref_count++; }

    void decrease_ref() { ref_count--; }

public:
    once_runner() {}

    ~once_runner() {
        spin_wait_while(ref_count, [&](std::int64_t value) { return value > 0; }, std::memory_order_acquire);
        if (is_ready.load(std::memory_order_relaxed)) {
            m_storage.~storage_t();
        }
    }

    void init() {
        new(&m_storage) storage_t();
    }

    void wait_for_readiness() {
        spin_wait_while_eq(is_ready, false);
    }

    std::uintptr_t to_bits() {
        return reinterpret_cast<std::uintptr_t>(this);
    }

    static once_runner* from_bits(std::uintptr_t bits) {
        __TBB_ASSERT( (bits & (max_nfs_size-1)) == 0, "invalid pointer, last log2(max_nfs_size) bits must be zero" );
        return reinterpret_cast<once_runner*>(bits);
    }

    template <typename F>
    void run_once(F&& f) {
        m_storage.m_arena.execute([&] {
            isolated_execute([&] {
                task_group_context context{ task_group_context::bound,
                    task_group_context::default_traits | task_group_context::concurrent_wait };

                function_stack_task<F> t{ std::forward<F>(f), m_storage.m_wait_context };

                is_ready.store(true, std::memory_order_release);
                execute_and_wait(t, context, m_storage.m_wait_context, context);
            });
        });
    }

    void assist() noexcept {
        m_storage.m_arena.execute([&] {
            isolated_execute([&] {
                // We do not want to get an exception from user functor on moonlighting threads.
                // The exception is handled with the winner thread
                task_group_context stub_context;
                wait(m_storage.m_wait_context, stub_context);
            });
        });
    }

};

class collaborative_once_flag : no_copy {
    enum state {uninitialized, done};
    std::atomic<std::uintptr_t> m_state{ state::uninitialized };

    template <typename Fn, typename... Args>
    friend void collaborative_call_once(collaborative_once_flag& flag, Fn&& f, Args&&... args);

    template <typename Fn>
    void do_collaborative_call_once(Fn&& f) {
        std::uintptr_t expected = m_state.load(std::memory_order_acquire);
        once_runner shared_runner;

        while (expected != state::done) {
            if (expected == state::uninitialized && m_state.compare_exchange_strong(expected, shared_runner.to_bits())) {
                // winner thread
                shared_runner.init();

                expected = shared_runner.to_bits();

                try_call([&] {
                    shared_runner.run_once(std::forward<Fn>(f));
                }).on_exception([&] {
                    while (!m_state.compare_exchange_strong(expected, state::uninitialized)) {
                        expected = shared_runner.to_bits();
                    }
                });

                while (!m_state.compare_exchange_strong(expected, state::done)) {
                    expected = shared_runner.to_bits();
                }
                
                break;
            } else {
                // moonlighting thread
                do {
                    auto max_value = expected | (max_nfs_size-1);
                    expected = spin_wait_while_eq(m_state, max_value);
                // "expected > state::done" prevents storing values, when state is uninitialized
                } while (expected > state::done && !m_state.compare_exchange_strong(expected, expected + 1));

                if (auto runner = once_runner::from_bits(expected & (std::size_t(-1) << bit_count))) {

                    once_runner::lifetime_tracker hold_ref_count{*runner};
                    m_state.fetch_sub(1);

                    runner->wait_for_readiness();

                    // The moonlighting threads are not expected to handle exceptions from user functor.
                    // Therefore, no exception is expected from assist().
                    runner->assist();
                }
            }
        }
    }
};


template <typename Fn, typename... Args>
void collaborative_call_once(collaborative_once_flag& flag, Fn&& fn, Args&&... args) {
#if __TBB_GCC_PARAMETER_PACK_IN_LAMBDAS_BROKEN
    // Using stored_pack to suppress bug in GCC 4.8
    // with parameter pack expansion in lambda
    auto stored_pack = save_pack(std::forward<Args>(args)...);
    auto func = [&] { call(std::forward<Fn>(fn), stored_pack); };
#else
    auto func = [&] { fn(std::forward<Args>(args)...); };
#endif
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

#endif // __TBB_PREVIEW_COLLABORATIVE_CALL_ONCE
#endif // __TBB_collaborative_call_once_H
