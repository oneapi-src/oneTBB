
#ifndef __TBB_collaborative_once_flag_H
#define __TBB_collaborative_once_flag_H

#include "tbb/task_arena.h"
#include "tbb/task_group.h"
#include "tbb/task.h"

#include <atomic>
#include <memory>
#include <mutex>

namespace tbb {

namespace detail {

class run_once {
    //! A tbb::empty_task allows provides something for other threads to wait on.
    tbb::task_arena my_arena;
    detail::d1::wait_context my_wait_context;
    mutable task_group_context my_context; ///< mutable so we can get the status in a const.

public:
    run_once()
        : my_arena(task_arena::attach{})
        , my_wait_context(0)
        , my_context(task_group_context::bound,
            task_group_context::default_traits | task_group_context::concurrent_wait) {}

    ~run_once() {
        // Caller is responsable for not pulling this out from under someone else.
        // We could use a std::shared_mutex though...
    }

    [[nodiscard]] bool is_canceling() const { return my_context.is_group_execution_cancelled(); }

    template <typename F>
    task_group_status run_and_wait(F&& f) {
        detail::d1::function_stack_task<F> t{ f, my_wait_context };
        my_wait_context.reserve();
        bool cancellation_status = false;
        auto func = [&]() {
            detail::try_call([&] { execute_and_wait(t, my_context, my_wait_context, my_context); }).on_completion([&] {
                cancellation_status = my_context.is_group_execution_cancelled();
                // Don't bother resetting the context. This isn't reusable.
            });
        };
        my_arena.execute([&func]() { return tbb::this_task_arena::isolate(func); });
        return cancellation_status ? canceled : complete;
    }

    /*! A call to wait() potentially participates in the work of run_and_wait().
     * If run_and_wait() throws or is canceled, this returns canceled.
     */
    task_group_status wait() {
        bool cancellation_status = false;
        auto f = [&]() {
            detail::try_call([&] {
                detail::d1::wait(my_wait_context, my_context);
            }).on_completion([&] {
                cancellation_status = my_context.is_group_execution_cancelled();
            });
        };
        my_arena.execute(f);
        return cancellation_status ? canceled : complete;
    }

};

} // namespace detail

class canceled_exception : public std::exception {};

// The concurrent_once_flag idea may be added to tbb at some point: oneapi-src/oneTBB#267
class collaborative_once_flag : detail::d0::no_copy {
    enum class state { uninitialized, in_progress, done };
    std::atomic<state> my_state = state::uninitialized;
    std::shared_ptr<detail::run_once> my_runner = nullptr;

    template <typename Fn>
    friend void collaborative_call_once(collaborative_once_flag& flag, Fn&& f);

    template <typename Fn>
    void do_collaborative_call_once(Fn&& f) {
        while (my_state.load(std::memory_order_acquire) !=
               state::done) { // Loop until someone else finishes or it's Done.
            state expected = state::uninitialized;
            if (my_state.compare_exchange_weak(expected, state::in_progress)) {
                // We won the race to be the primary thread.
                auto runner = std::make_shared<detail::run_once>();
                std::atomic_store_explicit(&my_runner, runner, std::memory_order_release);
                try {
                    tbb::task_group_status wait_result = runner->run_and_wait(f);
                    if (wait_result == tbb::task_group_status::canceled) {
                        throw canceled_exception(); // Translate cancelation to an exception.
                    }
                } catch (...) {
                    // Re-set and re-throw:
                    std::atomic_store_explicit(&my_runner, std::shared_ptr<detail::run_once>(nullptr),
                                               std::memory_order_release);
                    my_state.store(state::uninitialized, std::memory_order_release);
                    throw;
                }
                // Let go, mark done, and return:
                std::atomic_store_explicit(&my_runner, std::shared_ptr<detail::run_once>(nullptr),
                                           std::memory_order_release);
                my_state.store(state::done, std::memory_order_release);
                return;
            } else {
                // Get our own pointer so it can't go away while we aren't looking.
                if (auto runner = std::atomic_load_explicit(&my_runner, std::memory_order_acquire)) {
                    runner->wait();
                }
            }
        }
    }
};

template <typename Fn>
void collaborative_call_once(collaborative_once_flag& flag, Fn&& fn) {
    flag.do_collaborative_call_once(std::forward<Fn>(fn));
}

} // namespace tbb

#endif // __TBB_collaborative_once_flag_H
