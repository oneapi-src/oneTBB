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

#include "oneapi/tbb/task_group.h"
#include "threading_control.h"
#include "resource_manager.h"
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
    context_state_propagation_mutex_type::scoped_lock lock(the_context_state_propagation_mutex);
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

cache_aligned_unique_ptr<permit_manager> threading_control::make_permit_manager(unsigned workers_soft_limit) {
    cache_aligned_unique_ptr<permit_manager> pm{nullptr};
    if (true) {
        pm = make_cache_aligned_unique<market>(sizeof(market), workers_soft_limit);
    }

    return pm;
}

cache_aligned_unique_ptr<thread_dispatcher> threading_control::make_thread_dispatcher(threading_control* tc, unsigned workers_soft_limit, unsigned workers_hard_limit) {
    stack_size_type stack_size = global_control_active_value_unsafe(global_control::thread_stack_size);

    cache_aligned_unique_ptr<thread_dispatcher> td =
        make_cache_aligned_unique<thread_dispatcher>(sizeof(thread_dispatcher), *tc, workers_hard_limit, stack_size);
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

permit_manager_client* threading_control::register_client(arena& a) {
    {
        global_mutex_type::scoped_lock lock(g_threading_control_mutex);
        add_ref(/*public = */ false);
    }

    permit_manager_client* client = my_permit_manager->create_client(a, nullptr);
    my_thread_dispatcher->insert_ticket(client->get_ticket());

    return client;
}

void threading_control::unregister_client(permit_manager_client* client) {
    my_thread_dispatcher->remove_ticket(client->get_ticket());
    my_permit_manager->destroy_client(*client);
    release(/*public = */ false, /*blocking_terminate = */ false);
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
        int delta = thr_control->my_permit_manager->set_active_num_workers(soft_limit);
        thr_control->my_thread_dispatcher->adjust_job_count_estimate(delta);
        thr_control->release(/*is_public=*/false, /*blocking_terminate=*/false);
    }
}

void threading_control::adjust_demand(permit_manager_client& c, int delta, bool mandatory) {
    delta = my_permit_manager->adjust_demand(c, delta, mandatory);
    // c.my_adjust_demand_current_epoch.wait_until(target_epoch, /* context = */ target_epoch, std::memory_order_relaxed);
    // Must be called outside of any locks
    my_thread_dispatcher->my_server->adjust_job_count_estimate(delta);
    // c.my_adjust_demand_current_epoch.exchange(target_epoch + 1);
    // c.my_adjust_demand_current_epoch.notify_relaxed(target_epoch + 1);
}

void threading_control::enable_mandatory_concurrency(permit_manager_client* c) {
    int delta = my_permit_manager->enable_mandatory_concurrency(c);
    my_thread_dispatcher->adjust_job_count_estimate(delta);
}

void threading_control::mandatory_concurrency_disable(permit_manager_client* c) {
    int delta = my_permit_manager->mandatory_concurrency_disable(c);
    my_thread_dispatcher->adjust_job_count_estimate(delta);
}

} // r1
} // detail
} // tbb
