/*
    Copyright (c) 2005-2023 Intel Corporation

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

#include "task_dispatcher.h"
#include "governor.h"
#include "threading_control.h"
#include "arena_fixed_size.h"
#include "itt_notify.h"
#include "semaphore.h"
#include "waiters.h"
#include "oneapi/tbb/detail/_task.h"
#include "oneapi/tbb/info.h"
#include "oneapi/tbb/tbb_allocator.h"

#include <atomic>
#include <cstring>
#include <functional>

namespace tbb {
namespace prototype {
namespace r1 {

void arena_fixed_size::on_thread_leaving(unsigned ref_param) {
    //
    // Implementation of arena destruction synchronization logic contained various
    // bugs/flaws at the different stages of its evolution, so below is a detailed
    // description of the issues taken into consideration in the framework of the
    // current design.
    //
    // In case of using fire-and-forget tasks (scheduled via task::enqueue())
    // external thread is allowed to leave its arena before all its work is executed,
    // and market may temporarily revoke all workers from this arena. Since revoked
    // workers never attempt to reset arena state to EMPTY and cancel its request
    // to RML for threads, the arena object is destroyed only when both the last
    // thread is leaving it and arena's state is EMPTY (that is its external thread
    // left and it does not contain any work).
    // Thus resetting arena to EMPTY state (as earlier TBB versions did) should not
    // be done here (or anywhere else in the external thread to that matter); doing so
    // can result either in arena's premature destruction (at least without
    // additional costly checks in workers) or in unnecessary arena state changes
    // (and ensuing workers migration).
    //
    // A worker that checks for work presence and transitions arena to the EMPTY
    // state (in snapshot taking procedure arena::out_of_work()) updates
    // arena::my_pool_state first and only then arena::my_num_workers_requested.
    // So the check for work absence must be done against the latter field.
    //
    // In a time window between decrementing the active threads count and checking
    // if there is an outstanding request for workers. New worker thread may arrive,
    // finish remaining work, set arena state to empty, and leave decrementing its
    // refcount and destroying. Then the current thread will destroy the arena
    // the second time. To preclude it a local copy of the outstanding request
    // value can be stored before decrementing active threads count.
    //
    // But this technique may cause two other problem. When the stored request is
    // zero, it is possible that arena still has threads and they can generate new
    // tasks and thus re-establish non-zero requests. Then all the threads can be
    // revoked (as described above) leaving this thread the last one, and causing
    // it to destroy non-empty arena.
    //
    // The other problem takes place when the stored request is non-zero. Another
    // thread may complete the work, set arena state to empty, and leave without
    // arena destruction before this thread decrements the refcount. This thread
    // cannot destroy the arena either. Thus the arena may be "orphaned".
    //
    // In both cases we cannot dereference arena pointer after the refcount is
    // decremented, as our arena may already be destroyed.
    //
    // If this is the external thread, the market is protected by refcount to it.
    // In case of workers market's liveness is ensured by the RML connection
    // rundown protocol, according to which the client (i.e. the market) lives
    // until RML server notifies it about connection termination, and this
    // notification is fired only after all workers return into RML.
    //
    // Thus if we decremented refcount to zero we ask the market to check arena
    // state (including the fact if it is alive) under the lock.
    //

    __TBB_ASSERT(my_references.load(std::memory_order_relaxed) >= ref_param, "broken arena reference counter");

    // When there is no workers someone must free arena, as
    // without workers, no one calls out_of_work().
    if (ref_param == ref_external && !my_mandatory_concurrency.test()) {
        out_of_work();
    }

    tbb::detail::r1::threading_control* tc = my_threading_control;
    auto tc_client_snapshot = tc->prepare_client_destruction(my_tc_client);
    // Release our reference to sync with destroy_client
    unsigned remaining_ref = my_references.fetch_sub(ref_param, std::memory_order_release) - ref_param;
    // do not access `this` it might be destroyed already
    if (remaining_ref == 0) {
        if (tc->try_destroy_client(tc_client_snapshot)) {
            // We are requested to destroy ourself
            free_arena();
        }
    }
}


template <bool as_worker>
std::size_t arena_fixed_size::occupy_free_slot(tbb::detail::r1::thread_data& tls) {
    // Firstly, external threads try to occupy reserved slots
    std::size_t index = as_worker ? out_of_arena : occupy_free_slot_in_range( tls,  0, my_num_reserved_slots );
    if ( index == out_of_arena ) {
        // Secondly, all threads try to occupy all non-reserved slots
        index = occupy_free_slot_in_range(tls, my_num_reserved_slots, my_num_slots );
        // Likely this arena is already saturated
        if ( index == out_of_arena )
            return out_of_arena;
    }

    tbb::detail::r1::atomic_update( my_limit, (unsigned)(index + 1), std::less<unsigned>() );
    return index;
}


void arena_fixed_size::process(tbb::detail::r1::thread_data& tls) {
    tbb::detail::r1::governor::set_thread_data(tls); // TODO: consider moving to create_one_job.
    // TODO: Enable this assert
    //__TBB_ASSERT( is_alive(my_guard), nullptr);
    __TBB_ASSERT( my_num_slots >= 1, nullptr);

    std::size_t index = occupy_free_slot</*as_worker*/true>(tls);
    if (index == out_of_arena) {
        on_thread_leaving(ref_worker);
        return;
    }

    my_tc_client.get_pm_client()->register_thread();

    __TBB_ASSERT( index >= my_num_reserved_slots, "Workers cannot occupy reserved slots" );
    tls.attach_arena(*this, index);
    // worker thread enters the dispatch loop to look for a work
    tls.my_inbox.set_is_idle(true);
    if (tls.my_arena_slot->is_task_pool_published()) {
        tls.my_inbox.set_is_idle(false);
    }

    tbb::detail::r1::task_dispatcher& task_disp = tls.my_arena_slot->default_task_dispatcher();
    tls.enter_task_dispatcher(task_disp, calculate_stealing_threshold());
    __TBB_ASSERT(task_disp.can_steal(), nullptr);

    __TBB_ASSERT( !tls.my_last_observer, "There cannot be notified local observers when entering arena" );
    my_observers.notify_entry_observers(tls.my_last_observer, tls.my_is_worker);

    // Waiting on special object tied to this arena
    tbb::detail::r1::outermost_worker_waiter waiter(*this);
    tbb::detail::d1::task* t = tls.my_task_dispatcher->local_wait_for_all(nullptr, waiter);
    // For purposes of affinity support, the slot's mailbox is considered idle while no thread is
    // attached to it.
    tls.my_inbox.set_is_idle(true);

    __TBB_ASSERT_EX(t == nullptr, "Outermost worker must not leave dispatch loop with a task");
    __TBB_ASSERT(tbb::detail::r1::governor::is_thread_data_set(&tls), nullptr);
    __TBB_ASSERT(tls.my_task_dispatcher == &task_disp, nullptr);

    my_observers.notify_exit_observers(tls.my_last_observer, tls.my_is_worker);
    tls.my_last_observer = nullptr;

    tls.leave_task_dispatcher();

    // Arena slot detach (arena may be used in market::process)
    // TODO: Consider moving several calls below into a new method(e.g.detach_arena).
    tls.my_arena_slot->release();
    tls.my_arena_slot = nullptr;
    tls.my_inbox.detach();
    __TBB_ASSERT(tls.my_inbox.is_idle_state(true), nullptr);
   // __TBB_ASSERT(is_alive(my_guard), nullptr);

    my_tc_client.get_pm_client()->unregister_thread();

    // In contrast to earlier versions of TBB (before 3.0 U5) now it is possible
    // that arena may be temporarily left unpopulated by threads. See comments in
    // arena::on_thread_leaving() for more details.
    on_thread_leaving(ref_worker);
   // __TBB_ASSERT(tls.my_arena == this, "my_arena is used as a hint when searching the arena to join");
}

arena_fixed_size::arena_fixed_size(tbb::detail::r1::threading_control* control, unsigned num_slots, unsigned num_reserved_slots, unsigned priority_level) :
    arena(control, num_slots, num_reserved_slots, priority_level)  {

    //__TBB_ASSERT( !my_guard, "improperly allocated arena?" );
    //__TBB_ASSERT( sizeof(my_slots[0]) % cache_line_size()==0, "arena::slot size not multiple of cache line size" );
    //__TBB_ASSERT( is_aligned(this, cache_line_size()), "arena misaligned" );
#if 0
    my_threading_control = control;
    my_limit = 1;
    // Two slots are mandatory: for the external thread, and for 1 worker (required to support starvation resistant tasks).
    my_num_slots = num_arena_slots(num_slots, num_reserved_slots);
    my_num_reserved_slots = num_reserved_slots;
    my_max_num_workers = num_slots-num_reserved_slots;
    my_priority_level = priority_level;
    my_references = ref_external; // accounts for the external thread
    //my_observers.my_arena = this;
    my_co_cache.init(4 * num_slots);
    __TBB_ASSERT ( my_max_num_workers <= my_num_slots, nullptr);
    // Initialize the default context. It should be allocated before task_dispatch construction.
    my_default_ctx = new (tbb::detail::r1::cache_aligned_allocate(sizeof(tbb::detail::d1::task_group_context)))
        tbb::detail::d1::task_group_context{ tbb::detail::d1::task_group_context::isolated, tbb::detail::d1::task_group_context::fp_settings };
    // Construct slots. Mark internal synchronization elements for the tools.
    tbb::detail::r1::task_dispatcher* base_td_pointer = reinterpret_cast<tbb::detail::r1::task_dispatcher*>(my_slots + my_num_slots);
    for( unsigned i = 0; i < my_num_slots; ++i ) {
        // __TBB_ASSERT( !my_slots[i].my_scheduler && !my_slots[i].task_pool, nullptr);
        __TBB_ASSERT( !my_slots[i].task_pool_ptr, nullptr);
        __TBB_ASSERT( !my_slots[i].my_task_pool_size, nullptr);
        mailbox(i).construct();
        my_slots[i].init_task_streams(i);
        my_slots[i].my_default_task_dispatcher = new(base_td_pointer + i) tbb::detail::r1::task_dispatcher(this);
        my_slots[i].my_is_occupied.store(false, std::memory_order_relaxed);
    }
    my_fifo_task_stream.initialize(my_num_slots);
    my_resume_task_stream.initialize(my_num_slots);
    my_mandatory_requests = 0;
#endif
}

void arena_fixed_size::free_arena () {
    //__TBB_ASSERT( tbb::detail::r1::is_alive(my_guard), nullptr);
    __TBB_ASSERT( !my_references.load(std::memory_order_relaxed), "There are threads in the dying arena" );
    __TBB_ASSERT( !my_total_num_workers_requested && !my_num_workers_allotted, "Dying arena requests workers" );
    __TBB_ASSERT( is_empty(), "Inconsistent state of a dying arena" );
   // poison_value( my_guard );
    for ( unsigned i = 0; i < my_num_slots; ++i ) {
        // __TBB_ASSERT( !my_slots[i].my_scheduler, "arena slot is not empty" );
        // TODO: understand the assertion and modify
        // __TBB_ASSERT( my_slots[i].task_pool == EmptyTaskPool, nullptr);
        __TBB_ASSERT( my_slots[i].head == my_slots[i].tail, nullptr); // TODO: replace by is_quiescent_local_task_pool_empty
        my_slots[i].free_task_pool();
        mailbox(i).drain();
        my_slots[i].my_default_task_dispatcher->~task_dispatcher();
    }
    __TBB_ASSERT(my_fifo_task_stream.empty(), "Not all enqueued tasks were executed");
    __TBB_ASSERT(my_resume_task_stream.empty(), "Not all enqueued tasks were executed");
    // Cleanup coroutines/schedulers cache
    my_co_cache.cleanup();
    my_default_ctx->~task_group_context();
    tbb::detail::r1::cache_aligned_deallocate(my_default_ctx);

    // Clear enfources synchronization with observe(false)
    my_observers.clear();

    void* storage  = &mailbox(my_num_slots-1);
    __TBB_ASSERT( my_references.load(std::memory_order_relaxed) == 0, nullptr);
    this->~arena_fixed_size();
#if TBB_USE_ASSERT > 1
    std::memset( storage, 0, allocation_size(my_num_slots) );
#endif /* TBB_USE_ASSERT */
    tbb::detail::r1::cache_aligned_deallocate( storage );
}

bool arena_fixed_size::has_enqueued_tasks() {
    return !my_fifo_task_stream.empty();
}

void arena_fixed_size::request_workers(int mandatory_delta, int workers_delta, bool wakeup_threads) {
    my_threading_control->adjust_demand(my_tc_client, mandatory_delta, workers_delta);

    if (wakeup_threads) {
        // Notify all sleeping threads that work has appeared in the arena.
        get_waiting_threads_monitor().notify([&] (tbb::detail::r1::market_context context) {
            return this == context.my_arena_addr;
        });
    }
}

bool arena_fixed_size::has_tasks() {
    // TODO: rework it to return at least a hint about where a task was found; better if the task itself.
    std::size_t n = my_limit.load(std::memory_order_acquire);
    bool tasks_are_available = false;
    for (std::size_t k = 0; k < n && !tasks_are_available; ++k) {
        tasks_are_available = !my_slots[k].is_empty();
    }
    tasks_are_available = tasks_are_available || has_enqueued_tasks() || !my_resume_task_stream.empty();
    return tasks_are_available;
}

void arena_fixed_size::out_of_work() {
    // We should try unset my_pool_state first due to keep arena invariants in consistent state
    // Otherwise, we might have my_pool_state = false and my_mandatory_concurrency = true that is broken invariant
    bool disable_mandatory = my_mandatory_concurrency.try_clear_if([this] { return !has_enqueued_tasks(); });
    bool release_workers = my_pool_state.try_clear_if([this] { return !has_tasks(); });

    if (disable_mandatory || release_workers) {
        int mandatory_delta = disable_mandatory ? -1 : 0;
        int workers_delta = release_workers ? -(int)my_max_num_workers : 0;

        if (disable_mandatory && is_arena_workerless()) {
            // We had set workers_delta to 1 when enabled mandatory concurrency, so revert it now
            workers_delta = -1;
        }
        request_workers(mandatory_delta, workers_delta);
    }
}

void arena_fixed_size::set_top_priority(bool is_top_priority) {
    my_is_top_priority.store(is_top_priority, std::memory_order_relaxed);
}

bool arena_fixed_size::is_top_priority() const {
    return my_is_top_priority.load(std::memory_order_relaxed);
}

bool arena_fixed_size::try_join() {
    if (num_workers_active() < my_num_workers_allotted.load(std::memory_order_relaxed)) {
        my_references += arena_fixed_size::ref_worker;
        return true;
    }
    return false;
}

void arena_fixed_size::set_allotment(unsigned allotment) {
    if (my_num_workers_allotted.load(std::memory_order_relaxed) != allotment) {
        my_num_workers_allotted.store(allotment, std::memory_order_relaxed);
    }
}

int arena_fixed_size::update_concurrency(unsigned allotment) {
    int delta = allotment - my_num_workers_allotted.load(std::memory_order_relaxed);
    if (delta != 0) {
        my_num_workers_allotted.store(allotment, std::memory_order_relaxed);
    }
    return delta;
}

std::pair<int, int> arena_fixed_size::update_request(int mandatory_delta, int workers_delta) {
    __TBB_ASSERT(-1 <= mandatory_delta && mandatory_delta <= 1, nullptr);

    int min_workers_request = 0;
    int max_workers_request = 0;

    // Calculate min request
    my_mandatory_requests += mandatory_delta;
    min_workers_request = my_mandatory_requests > 0 ? 1 : 0;

    // Calculate max request
    my_total_num_workers_requested += workers_delta;
    // Clamp worker request into interval [0, my_max_num_workers]
    max_workers_request = tbb::detail::d0::clamp(my_total_num_workers_requested, 0,
        min_workers_request > 0 && is_arena_workerless() ? 1 : (int)my_max_num_workers);

    return { min_workers_request, max_workers_request };
}

tbb::detail::r1::thread_control_monitor& arena_fixed_size::get_waiting_threads_monitor() {
    return my_threading_control->get_waiting_threads_monitor();
}

arena_fixed_size& arena_fixed_size::allocate_arena(tbb::detail::r1::threading_control* control, unsigned num_slots, unsigned num_reserved_slots,
    unsigned priority_level)
{
    //__TBB_ASSERT(sizeof(base_type) + sizeof(arena_slot) == sizeof(arena), "All arena data fields must go to arena_base");
    //__TBB_ASSERT(sizeof(base_type) % cache_line_size() == 0, "arena slots area misaligned: wrong padding");
    //__TBB_ASSERT(sizeof(mail_outbox) == max_nfs_size, "Mailbox padding is wrong");
    std::size_t n = allocation_size(num_arena_slots(num_slots, num_reserved_slots));
    unsigned char* storage = (unsigned char*)tbb::detail::r1::cache_aligned_allocate(n);
    // Zero all slots to indicate that they are empty
    std::memset(storage, 0, n);

    return *new(storage + num_arena_slots(num_slots, num_reserved_slots) * sizeof(tbb::detail::r1::mail_outbox))
        arena_fixed_size(control, num_slots, num_reserved_slots, priority_level);
}

arena_fixed_size& arena_fixed_size::create(tbb::detail::r1::threading_control* control, unsigned num_slots, unsigned num_reserved_slots, unsigned arena_priority_level, tbb::detail::d1::constraints constraints) {
    __TBB_ASSERT(num_slots > 0, NULL);
    __TBB_ASSERT(num_reserved_slots <= num_slots, NULL);
    // Add public market reference for an external thread/task_arena (that adds an internal reference in exchange).
    arena_fixed_size& a = arena_fixed_size::allocate_arena(control, num_slots, num_reserved_slots, arena_priority_level);
    a.my_tc_client = control->create_client(a);
    // We should not publish arena until all fields are initialized
    control->publish_client(a.my_tc_client, constraints);
    return a;
}

} // namespace r1
} // namespace prototype
} // namespace tbb

// Enable task_arena_fixed_size.h
#include "oneapi/tbb/task_arena_fixed_size.h" // task_arena_base


namespace tbb {
namespace prototype {
namespace r1 {

struct task_arena_fixed_size_impl {
    static void initialize(d1::task_arena_fixed_size_base&);
    static void terminate(d1::task_arena_fixed_size_base&);
};
void __TBB_EXPORTED_FUNC initialize(d1::task_arena_fixed_size_base& ta) {
    task_arena_fixed_size_impl::initialize(ta);
}
void __TBB_EXPORTED_FUNC terminate(d1::task_arena_fixed_size_base& ta) {
    task_arena_fixed_size_impl::terminate(ta);
}

void task_arena_fixed_size_impl::initialize(d1::task_arena_fixed_size_base& ta) {
    // Enforce global market initialization to properly initialize soft limit
    (void)tbb::detail::r1::governor::get_thread_data();
    tbb::detail::d1::constraints arena_constraints;

    //__TBB_ASSERT(ta.my_arena.load(std::memory_order_relaxed) == nullptr, "Arena already initialized");
    //unsigned priority_level = arena_priority_level(ta.my_priority);
    tbb::detail::r1::threading_control* thr_control = tbb::detail::r1::threading_control::register_public_reference();
    arena_fixed_size& a = arena_fixed_size::create(thr_control, unsigned(ta.my_max_concurrency), ta.my_num_reserved_slots, unsigned(ta.my_priority), arena_constraints);

    ta.my_arena.store(&a, std::memory_order_release);
}

void task_arena_fixed_size_impl::terminate(d1::task_arena_fixed_size_base& ta) {
    arena_fixed_size* a = ta.my_arena.load(std::memory_order_relaxed);
    tbb::detail::assert_pointer_valid(a);
    tbb::detail::r1::threading_control::unregister_public_reference(/*blocking_terminate=*/false);
    a->on_thread_leaving(arena_fixed_size::ref_external);
    ta.my_arena.store(nullptr, std::memory_order_relaxed);
}


} // namespace r1
} // namespace prototype
} // namespace tbb

