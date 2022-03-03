/*
    Copyright (c) 2005-2022 Intel Corporation

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

#ifndef _TBB_threading_cont_H
#define _TBB_threading_cont_H

#include "oneapi/tbb/mutex.h"
#include "oneapi/tbb/global_control.h"
#include "thread_dispatcher.h"
#include "resource_manager.h"
#include "intrusive_list.h"
#include "main.h"

#include <memory>

namespace tbb {
namespace detail {
namespace r1 {


struct cache_aligned_deleter {
    template <typename T>
    void operator() (T* ptr) const {
        ptr->~T();
        cache_aligned_deallocate(ptr);
    }
};

template <typename T>
using cache_aligned_unique_ptr = std::unique_ptr<T, cache_aligned_deleter>;

template <typename T, typename ...Args>
cache_aligned_unique_ptr<T> make_cache_aligned_unique(std::size_t size, Args&& ...args) {
    void* storage = cache_aligned_allocate(size);
    std::memset(storage, 0, size);
    return cache_aligned_unique_ptr<T>(new (storage) T(std::forward<Args>(args)...));
}




class arena;
class thread_data;
class permit_manager_client;

void global_control_lock();
void global_control_unlock();
std::size_t global_control_active_value_unsafe(d1::global_control::parameter);


class cancellation_disseminator {
public:
    //! Finds all contexts affected by the state change and propagates the new state to them.
    /*  The propagation is relayed to the cancellation_disseminator because tasks created by one
        external thread can be passed to and executed by other external threads. This means
        that context trees can span several arenas at once and thus state change
        propagation cannot be generally localized to one arena only.
    */
    bool propagate_task_group_state(std::atomic<uint32_t> d1::task_group_context::*mptr_state, d1::task_group_context& src, uint32_t new_state);

    void register_thread(thread_data& td) {
        threads_list_mutex_type::scoped_lock lock(my_threads_list_mutex);
        my_threads_list.push_front(td);
    }

    void unregister_thread(thread_data& td) {
        threads_list_mutex_type::scoped_lock lock(my_threads_list_mutex);
        my_threads_list.remove(td);
    }

private:
    using thread_data_list_type = intrusive_list<thread_data>;
    using threads_list_mutex_type = d1::mutex;

    threads_list_mutex_type my_threads_list_mutex;
    thread_data_list_type my_threads_list{};
};

class threading_control {
public:
    using global_mutex_type = d1::mutex;

    threading_control() : my_public_ref_count(1), my_ref_count(1)
    {}

private:
    void add_ref(bool is_public) {
        ++my_ref_count;
        if (is_public) {
            my_public_ref_count++;
        }
    }

    bool remove_ref(bool is_public) {
        if (is_public) {
            __TBB_ASSERT(g_threading_control.get() == this, "Global threading controle instance was destroyed prematurely?");
            __TBB_ASSERT(my_public_ref_count.load(std::memory_order_relaxed), nullptr);
            --my_public_ref_count;
        }

        bool is_last_ref = --my_ref_count == 0;
        if (is_last_ref) {
            __TBB_ASSERT(!my_public_ref_count.load(std::memory_order_relaxed), nullptr);
            g_threading_control.release();
        }

        return is_last_ref;
    }

    static threading_control* get_threading_control(bool is_public) {
        threading_control* control = g_threading_control.get();
        if (control) {
           control->add_ref(is_public);
        }

        return control;
    }

    static unsigned calc_workers_soft_limit(unsigned workers_soft_limit, unsigned workers_hard_limit) {
        unsigned soft_limit = global_control_active_value_unsafe(global_control::max_allowed_parallelism);
        if (soft_limit) {
            workers_soft_limit = soft_limit - 1;
        } else {
            // if user set no limits (yet), use threading control's parameter
            workers_soft_limit = max(governor::default_num_threads() - 1, workers_soft_limit);
        }

        if (workers_soft_limit >= workers_hard_limit) {
            workers_soft_limit = workers_hard_limit - 1;
        }

        return workers_soft_limit;
    }

    static std::pair<unsigned, unsigned> calculate_workers_limits() {
        // Expecting that 4P is suitable for most applications.
        // Limit to 2P for large thread number.
        // TODO: ask RML for max concurrency and possibly correct hard_limit
        unsigned factor = governor::default_num_threads() <= 128 ? 4 : 2;

        // The requested number of threads is intentionally not considered in
        // computation of the hard limit, in order to separate responsibilities
        // and avoid complicated interactions between global_control and task_scheduler_init.
        // The threading control guarantees that at least 256 threads might be created.
        unsigned workers_app_limit = global_control_active_value_unsafe(global_control::max_allowed_parallelism);
        unsigned workers_hard_limit = max(max(factor * governor::default_num_threads(), 256u), workers_app_limit);
        unsigned workers_soft_limit = calc_workers_soft_limit(governor::default_num_threads(), workers_hard_limit);
        
        return std::make_pair(workers_soft_limit, workers_hard_limit);
    }

    static cache_aligned_unique_ptr<permit_manager> make_permit_manager(unsigned workers_soft_limit);

    static cache_aligned_unique_ptr<thread_dispatcher> make_thread_dispatcher(threading_control* control, unsigned workers_soft_limit, unsigned workers_hard_limit);

    static threading_control* create_threading_control() {
        threading_control* thr_control{nullptr};

        try_call([&] {
            // Global control should be locked before threading_control
            global_control_lock();
            global_mutex_type::scoped_lock lock(g_threading_control_mutex);

            thr_control = get_threading_control(/*public = */ true);
            if (thr_control != nullptr) {
                return;
            }

            thr_control = new (cache_aligned_allocate(sizeof(threading_control))) threading_control();

            unsigned workers_soft_limit{}, workers_hard_limit{};
            std::tie(workers_soft_limit, workers_hard_limit) = calculate_workers_limits();

            thr_control->my_permit_manager = make_permit_manager(workers_soft_limit);
            thr_control->my_thread_dispatcher = make_thread_dispatcher(thr_control, workers_soft_limit, workers_hard_limit);
            thr_control->my_cancellation_disseminator = make_cache_aligned_unique<cancellation_disseminator>(sizeof(cancellation_disseminator));
            __TBB_InitOnce::add_ref();

            if (global_control_active_value_unsafe(global_control::scheduler_handle)) {
                ++thr_control->my_public_ref_count;
                ++thr_control->my_ref_count;
            }

            g_threading_control.reset(thr_control);
        }).on_completion([] { global_control_unlock(); });

        return thr_control;
    }

    void destroy () {
        cache_aligned_deleter deleter;
        deleter(this);
        __TBB_InitOnce::remove_ref();
    }

    void wait_last_reference(global_mutex_type::scoped_lock& lock) {
        while (my_public_ref_count.load(std::memory_order_relaxed) == 1 && my_ref_count.load(std::memory_order_relaxed) > 1) {
            lock.release();
            // To guarantee that request_close_connection() is called by the last external thread, we need to wait till all
            // references are released. Re-read my_public_ref_count to limit waiting if new external threads are created.
            // Theoretically, new private references to the threading control can be added during waiting making it potentially
            // endless.
            // TODO: revise why the weak scheduler needs threading control's pointer and try to remove this wait.
            // Note that the threading control should know about its schedulers for cancellation/exception/priority propagation,
            // see e.g. task_group_context::cancel_group_execution()
            while (my_public_ref_count.load(std::memory_order_acquire) == 1 && my_ref_count.load(std::memory_order_acquire) > 1) {
                yield();
            }
            lock.acquire(g_threading_control_mutex);
        }
    }

    bool release(bool is_public, bool blocking_terminate);

public:
    static threading_control* register_public_reference() {
        threading_control* control{nullptr};
        global_mutex_type::scoped_lock lock(g_threading_control_mutex);
        control = get_threading_control(/*public = */ true);
        if (!control) {
            // We are going to create threading_control, we should acquire mutexes in right order
            lock.release();
            control = create_threading_control();
        }

        return control;
    }

    static bool unregister_public_reference(bool blocking_terminate) {
        __TBB_ASSERT(g_threading_control.get(), "Threading control should exist until last public reference");
        __TBB_ASSERT(g_threading_control->my_public_ref_count.load(std::memory_order_relaxed), nullptr);
        return g_threading_control->release(/*public = */ true, /*blocking_terminate = */ blocking_terminate);
    }


    permit_manager_client* register_client(arena& a);

    void unregister_client(permit_manager_client* client);

    permit_manager* get_permit_manager() {
        return my_permit_manager.get();
    }

    static bool register_lifetime_control() {
        global_mutex_type::scoped_lock lock(g_threading_control_mutex);
        return get_threading_control(/*public = */ true) != nullptr;
    }

    static bool unregister_lifetime_control(bool blocking_terminate) {
        threading_control* thr_control{nullptr};
        {
            global_mutex_type::scoped_lock lock(g_threading_control_mutex);
            thr_control = g_threading_control.get();
        }

        bool released{true};
        if (thr_control) {
            released = thr_control->release(/*public = */ true, /*blocking_terminate = */ blocking_terminate);
        }

        return released;
    }

    static void set_active_num_workers(unsigned soft_limit);

    static bool is_present() {
        global_mutex_type::scoped_lock lock(g_threading_control_mutex);
        return g_threading_control.get() != nullptr;
    }

    void register_thread(thread_data& td) {
        my_cancellation_disseminator->register_thread(td);
    }

    void unregister_thread(thread_data& td) {
        my_cancellation_disseminator->unregister_thread(td);
    }

    std::size_t worker_stack_size();

    static unsigned max_num_workers();

    void adjust_demand(permit_manager_client&, int delta, bool mandatory);
    void enable_mandatory_concurrency(permit_manager_client* c);
    void mandatory_concurrency_disable(permit_manager_client* c);

    void propagate_task_group_state(std::atomic<uint32_t> d1::task_group_context::*mptr_state, d1::task_group_context& src, uint32_t new_state) {
        my_cancellation_disseminator->propagate_task_group_state(mptr_state, src, new_state);
    }
private:
    friend class thread_dispatcher;

    static cache_aligned_unique_ptr<threading_control> g_threading_control;

    //! Mutex guarding creation/destruction of g_threading_control, insertions/deletions in my_arenas, and cancellation propagation
    static global_mutex_type g_threading_control_mutex;

    //! ABA prevention marker to assign to newly created arenas
    std::atomic<uintptr_t> my_arenas_aba_epoch{0};

    //! Count of external threads attached
    std::atomic<unsigned> my_public_ref_count{0};

    //! Reference count controlling threading_control object lifetime
    std::atomic<unsigned> my_ref_count{0};

    cache_aligned_unique_ptr<permit_manager> my_permit_manager{nullptr};
    cache_aligned_unique_ptr<thread_dispatcher> my_thread_dispatcher{nullptr};
    cache_aligned_unique_ptr<cancellation_disseminator> my_cancellation_disseminator{nullptr};
};

} // r1
} // detail
} // tbb


#endif // _TBB_threading_cont_H
