/*
    Copyright (c) 2022 Intel Corporation

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

#include "oneapi/tbb/task_group.h"
#include "threading_control.h"
#include "permit_manager.h"
#include "market.h"
#include "thread_dispatcher.h"
#include "governor.h"
#include "clients.h"

namespace tbb {
namespace detail {
namespace r1 {

bool cancellation_disseminator::propagate_task_group_state(std::atomic<uint32_t> d1::task_group_context::*mptr_state,
                                                           d1::task_group_context& src, uint32_t new_state)
{
    if (src.my_may_have_children.load(std::memory_order_relaxed) != d1::task_group_context::may_have_children) {
        return true;
    }

    // The whole propagation algorithm is under the lock in order to ensure correctness
    // in case of concurrent state changes at the different levels of the context tree.
    // See comment at the bottom of scheduler.cpp
    threads_list_mutex_type::scoped_lock lock(my_threads_list_mutex);
    if ((src.*mptr_state).load(std::memory_order_relaxed) != new_state) {
        // Another thread has concurrently changed the state. Back down.
        return false;
    }

    // Advance global state propagation epoch
    ++the_context_state_propagation_epoch;
    // Propagate to all workers and external threads and sync up their local epochs with the global one
    // The whole propagation sequence is locked, thus no contention is expected
    for (thread_data_list_type::iterator it = my_threads_list.begin(); it != my_threads_list.end(); it++) {
        it->propagate_task_group_state(mptr_state, src, new_state);
    }

    return true;
}

d1::cache_aligned_unique_ptr<permit_manager> threading_control::make_permit_manager(unsigned workers_soft_limit) {
    return d1::make_cache_aligned_unique<market>(workers_soft_limit);
}

d1::cache_aligned_unique_ptr<thread_dispatcher> threading_control::make_thread_dispatcher(threading_control* tc, unsigned workers_soft_limit, unsigned workers_hard_limit) {
    stack_size_type stack_size = global_control_active_value_unsafe(global_control::thread_stack_size);

    d1::cache_aligned_unique_ptr<thread_dispatcher> td =
        d1::make_cache_aligned_unique<thread_dispatcher>(*tc, workers_hard_limit, stack_size);
    // This check relies on the fact that for shared RML default_concurrency == max_concurrency
    if (!governor::UsePrivateRML && td->my_server->default_concurrency() < workers_soft_limit) {
        runtime_warning("RML might limit the number of workers to %u while %u is requested.\n",
            td->my_server->default_concurrency(), workers_soft_limit);
    }

    return td;
}

bool threading_control::release(bool is_public, bool blocking_terminate) {
    bool do_release = false;
    {
        global_mutex_type::scoped_lock lock(g_threading_control_mutex);
        if (blocking_terminate) {
            __TBB_ASSERT(is_public, "Only an object with a public reference can request the blocking terminate");
            wait_last_reference(lock);
        }
        do_release = remove_ref(is_public);
    }

    if (do_release) {
        __TBB_ASSERT(!my_public_ref_count.load(std::memory_order_relaxed), "No public references remain if we remove the threading control.");
        // inform RML that blocking termination is required
        my_thread_dispatcher->my_join_workers = blocking_terminate;
        my_thread_dispatcher->my_server->request_close_connection();
        return blocking_terminate;
    }
    return false;
}

threading_control_client threading_control::create_client(arena& a) {
    {
        global_mutex_type::scoped_lock lock(g_threading_control_mutex);
        add_ref(/*public = */ false);
    }

    pm_client* pm_client = my_permit_manager->create_client(a, nullptr);
    thread_dispatcher_client* td_client = my_thread_dispatcher->create_client(a);

    return {pm_client, td_client};
}

client_deleter threading_control::prepare_destroy(threading_control_client client) {
    thread_dispatcher_client* td_client = client.my_thread_dispatcher_client;
    return {td_client->get_aba_epoch(), td_client->priority_level(), td_client, static_cast<pm_client*>(client.my_permit_manager_client)};
}

bool threading_control::try_destroy_client(client_deleter deleter) {
    if (my_thread_dispatcher->try_unregister_client(deleter.my_td_client, deleter.aba_epoch, deleter.priority_level)) {
        my_permit_manager->destroy_client(*deleter.my_pm_client);
        release(/*public = */ false, /*blocking_terminate = */ false);
        return true;
    }
    return false;
}

void threading_control::publish_client(threading_control_client client) {
    my_permit_manager->register_client(static_cast<pm_client*>(client.my_permit_manager_client));
    my_thread_dispatcher->register_client(client.my_thread_dispatcher_client);
}

std::size_t threading_control::worker_stack_size() {
    return my_thread_dispatcher->worker_stack_size();
}

unsigned threading_control::max_num_workers() {
    global_mutex_type::scoped_lock lock(g_threading_control_mutex);
    return g_threading_control ? g_threading_control->my_thread_dispatcher->my_num_workers_hard_limit : 0;
}

void threading_control::set_active_num_workers(unsigned soft_limit) {
    threading_control* thr_control = get_threading_control(/*public = */ false);
    if (thr_control != nullptr) {
        __TBB_ASSERT(soft_limit <= thr_control->my_thread_dispatcher->my_num_workers_hard_limit, nullptr);
        thr_control->my_concurrency_tracker->set_active_num_workers(soft_limit);
        thr_control->my_permit_manager->set_active_num_workers(soft_limit);
        thr_control->release(/*is_public=*/false, /*blocking_terminate=*/false);
    }
}

bool threading_control::check_client_priority(threading_control_client client) {
    return static_cast<pm_client*>(client.my_permit_manager_client)->is_top_priority();
}

void threading_control::adjust_demand(threading_control_client tc_client, int mandatory_delta, int workers_delta) {
    pm_client& c = *static_cast<pm_client*>(tc_client.my_permit_manager_client);
    my_concurrency_tracker->register_mandatory_request(mandatory_delta);
    my_permit_manager->adjust_demand(c, mandatory_delta, workers_delta);
}

void threading_control::update_worker_request(int delta) {
    my_thread_dispatcher->adjust_job_count_estimate(delta);
}

threading_control* threading_control::create_threading_control() {
    threading_control* thr_control{ nullptr };

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
        thr_control->my_thread_request_serializer =
            d1::make_cache_aligned_unique<thread_request_serializer>(*thr_control->my_thread_dispatcher, workers_soft_limit);
        thr_control->my_concurrency_tracker =
            d1::make_cache_aligned_unique<concurrency_tracker>(*thr_control->my_thread_request_serializer.get());
        thr_control->my_permit_manager->set_thread_request_observer(*thr_control->my_thread_request_serializer);

        thr_control->my_cancellation_disseminator = d1::make_cache_aligned_unique<cancellation_disseminator>();
        __TBB_InitOnce::add_ref();

        if (global_control_active_value_unsafe(global_control::scheduler_handle)) {
            ++thr_control->my_public_ref_count;
            ++thr_control->my_ref_count;
        }

        g_threading_control = thr_control;
        }).on_completion([] { global_control_unlock(); });

        return thr_control;
}

// ------------------------ thread_request_serializer ------------------------

thread_request_serializer::thread_request_serializer(thread_dispatcher& td, int soft_limit)
    : my_thread_dispatcher{ td }
    , my_soft_limit{ soft_limit }
{}

void thread_request_serializer::update(int delta) {
    constexpr std::uint64_t delta_mask = 0xFFFF;
    constexpr std::uint64_t counter_value = delta_mask + 1;

    int prev_pending_delta = my_pending_delta.fetch_add(counter_value + delta);

    // There is a pseudo request aggregator, so only thread that see pending_delta_base in my_pending_delta
    // Will enter to critical section and call adjust_job_count_estimate
    if (prev_pending_delta == pending_delta_base) {
        mutex_type::scoped_lock lock(my_mutex);
        delta = int(my_pending_delta.exchange(pending_delta_base) & delta_mask) - int(pending_delta_base);
        my_total_request += delta;

        delta = limit_delta(delta, my_soft_limit, my_total_request);
        my_thread_dispatcher.adjust_job_count_estimate(delta);
    }
}

void thread_request_serializer::set_active_num_workers(int soft_limit) {
    mutex_type::scoped_lock lock(my_mutex);
    int delta = soft_limit - my_soft_limit;
    delta = limit_delta(delta, my_total_request, soft_limit);
    my_thread_dispatcher.adjust_job_count_estimate(delta);
    my_soft_limit = soft_limit;
}

int thread_request_serializer::limit_delta(int delta, int limit, int new_value) {
    int prev_value = new_value - delta;

    bool above_limit = prev_value >= limit && new_value >= limit;
    bool below_limit = prev_value <= limit && new_value <= limit;
    enum request_type { ABOVE_LIMIT, CROSS_LIMIT, BELOW_LIMIT };
    request_type request = above_limit ? ABOVE_LIMIT : below_limit ? BELOW_LIMIT : CROSS_LIMIT;

    switch (request) {
    case ABOVE_LIMIT:
        delta = 0;
        break;
    case CROSS_LIMIT:
        delta = delta > 0 ? limit - prev_value : new_value - limit;
        break;
    case BELOW_LIMIT:
        // No chagnes to delta
        break;
    }

    return delta;
}

} // r1
} // detail
} // tbb
