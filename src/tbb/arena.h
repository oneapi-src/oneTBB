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

#ifndef _TBB_arena_H
#define _TBB_arena_H

#include <atomic>
#include <cstring>

#include "oneapi/tbb/detail/_task.h"

#include "scheduler_common.h"
#include "intrusive_list.h"
#include "task_stream.h"
#include "arena_slot.h"
#include "rml_tbb.h"
#include "mailbox.h"
#include "governor.h"
#include "concurrent_monitor.h"
#include "observer_proxy.h"
#include "oneapi/tbb/spin_mutex.h"
#include "market_concurrent_monitor.h"

#include "resource_manager.h"

namespace tbb {
namespace detail {
namespace r1 {

class task_dispatcher;
class thread_dispatcher_client;
class permit_manager_client;
class task_group_context;
class threading_control;
class allocate_root_with_context_proxy;

struct threading_control_client {
    permit_manager_client* my_permit_manager_client;
    thread_dispatcher_client* my_thread_dispatcher_client;
};

#if __TBB_ARENA_BINDING
class numa_binding_observer;
#endif /*__TBB_ARENA_BINDING*/

//! Bounded coroutines cache LIFO ring buffer
class arena_co_cache {
    //! Ring buffer storage
    task_dispatcher** my_co_scheduler_cache;
    //! Current cache index
    unsigned my_head;
    //! Cache capacity for arena
    unsigned my_max_index;
    //! Accessor lock for modification operations
    tbb::spin_mutex my_co_cache_mutex;

    unsigned next_index() {
        return ( my_head == my_max_index ) ? 0 : my_head + 1;
    }

    unsigned prev_index() {
        return ( my_head == 0 ) ? my_max_index : my_head - 1;
    }

    bool internal_empty() {
        return my_co_scheduler_cache[prev_index()] == nullptr;
    }

    void internal_task_dispatcher_cleanup(task_dispatcher* to_cleanup) {
        to_cleanup->~task_dispatcher();
        cache_aligned_deallocate(to_cleanup);
    }

public:
    void init(unsigned cache_capacity) {
        std::size_t alloc_size = cache_capacity * sizeof(task_dispatcher*);
        my_co_scheduler_cache = (task_dispatcher**)cache_aligned_allocate(alloc_size);
        std::memset( my_co_scheduler_cache, 0, alloc_size );
        my_head = 0;
        my_max_index = cache_capacity - 1;
    }

    void cleanup() {
        while (task_dispatcher* to_cleanup = pop()) {
            internal_task_dispatcher_cleanup(to_cleanup);
        }
        cache_aligned_deallocate(my_co_scheduler_cache);
    }

    //! Insert scheduler to the current available place.
    //! Replace an old value, if necessary.
    void push(task_dispatcher* s) {
        task_dispatcher* to_cleanup = nullptr;
        {
            tbb::spin_mutex::scoped_lock lock(my_co_cache_mutex);
            // Check if we are replacing some existing buffer entrance
            if (my_co_scheduler_cache[my_head] != nullptr) {
                to_cleanup = my_co_scheduler_cache[my_head];
            }
            // Store the cached value
            my_co_scheduler_cache[my_head] = s;
            // Move head index to the next slot
            my_head = next_index();
        }
        // Cleanup replaced buffer if any
        if (to_cleanup) {
            internal_task_dispatcher_cleanup(to_cleanup);
        }
    }

    //! Get a cached scheduler if any
    task_dispatcher* pop() {
        tbb::spin_mutex::scoped_lock lock(my_co_cache_mutex);
        // No cached coroutine
        if (internal_empty()) {
            return nullptr;
        }
        // Move head index to the currently available value
        my_head = prev_index();
        // Retrieve the value from the buffer
        task_dispatcher* to_return = my_co_scheduler_cache[my_head];
        // Clear the previous entrance value
        my_co_scheduler_cache[my_head] = nullptr;
        return to_return;
    }
};

struct stack_anchor_type {
    stack_anchor_type() = default;
    stack_anchor_type(const stack_anchor_type&) = delete;
};

#if __TBB_ENQUEUE_ENFORCED_CONCURRENCY
class atomic_flag {
    static const std::uintptr_t SET = 1;
    static const std::uintptr_t EMPTY = 0;
    std::atomic<std::uintptr_t> my_state;
public:
    bool test_and_set() {
        std::uintptr_t state = my_state.load(std::memory_order_acquire);
        switch (state) {
        case SET:
            return false;
        default: /* busy */
            if (my_state.compare_exchange_strong(state, SET)) {
                // We interrupted clear transaction
                return false;
            }
            if (state != EMPTY) {
                // We lost our epoch
                return false;
            }
            // We are too late but still in the same epoch
            __TBB_fallthrough;
        case EMPTY:
            return my_state.compare_exchange_strong(state, SET);
        }
    }
    template <typename Pred>
    bool try_clear_if(Pred&& pred) {
        std::uintptr_t busy = std::uintptr_t(&busy);
        std::uintptr_t state = my_state.load(std::memory_order_acquire);
        if (state == SET && my_state.compare_exchange_strong(state, busy)) {
            if (pred()) {
                return my_state.compare_exchange_strong(busy, EMPTY);
            }
            // The result of the next operation is discarded, always false should be returned.
            my_state.compare_exchange_strong(busy, SET);
        }
        return false;
    }
    void clear() {
        my_state.store(EMPTY, std::memory_order_release);
    }
    bool test() {
        return my_state.load(std::memory_order_acquire) != EMPTY;
    }
};
#endif

//! The structure of an arena, except the array of slots.
/** Separated in order to simplify padding.
    Intrusive list node base class is used by market to form a list of arenas. **/
// TODO: Analyze arena_base cache lines placement
class arena_base : padded<intrusive_list_node> {
protected:
    //! The number of workers that have been marked out by the resource manager to service the arena.
    std::atomic<unsigned> my_num_workers_allotted;   // heavy use in stealing loop

    //! Reference counter for the arena.
    /** Worker and external thread references are counted separately: first several bits are for references
        from external thread threads or explicit task_arenas (see arena::ref_external_bits below);
        the rest counts the number of workers servicing the arena. */
    std::atomic<unsigned> my_references;     // heavy use in stealing loop

    //! The maximal number of currently busy slots.
    std::atomic<unsigned> my_limit;          // heavy use in stealing loop

    //! Task pool for the tasks scheduled via task::enqueue() method.
    /** Such scheduling guarantees eventual execution even if
        - new tasks are constantly coming (by extracting scheduled tasks in
          relaxed FIFO order);
        - the enqueuing thread does not call any of wait_for_all methods. **/
    task_stream<front_accessor> my_fifo_task_stream; // heavy use in stealing loop

    //! Task pool for the tasks scheduled via tbb::resume() function.
    task_stream<front_accessor> my_resume_task_stream; // heavy use in stealing loop

#if __TBB_PREVIEW_CRITICAL_TASKS
    //! Task pool for the tasks with critical property set.
    /** Critical tasks are scheduled for execution ahead of other sources (including local task pool
        and even bypassed tasks) unless the thread already executes a critical task in an outer
        dispatch loop **/
    // used on the hot path of the task dispatch loop
    task_stream<back_nonnull_accessor> my_critical_task_stream;
#endif

    //! The total number of workers that are requested from the resource manager.
    int my_total_num_workers_requested;

    //! The number of workers that are really requested from the resource manager.
    //! Possible values are in [0, my_max_num_workers]
    int my_num_workers_requested;

    //! The index in the array of per priority lists of arenas this object is in.
    /*const*/ unsigned my_priority_level;

    //! Current task pool state and estimate of available tasks amount.
    /** The estimate is either 0 (SNAPSHOT_EMPTY) or infinity (SNAPSHOT_FULL).
        Special state is "busy" (any other unsigned value).
        Note that the implementation of arena::is_busy_or_empty() requires
        my_pool_state to be unsigned. */
    using pool_state_t = std::uintptr_t ;
    std::atomic<pool_state_t> my_pool_state;

    //! The list of local observers attached to this arena.
    observer_list my_observers;

#if __TBB_ARENA_BINDING
    //! Pointer to internal observer that allows to bind threads in arena to certain NUMA node.
    numa_binding_observer* my_numa_binding_observer;
#endif /*__TBB_ARENA_BINDING*/

    // Below are rarely modified members

    threading_control* my_threading_control;

    //! Default task group context.
    d1::task_group_context* my_default_ctx;

    //! Waiting object for external threads that cannot join the arena.
    concurrent_monitor my_exit_monitors;

    //! Coroutines (task_dispathers) cache buffer
    arena_co_cache my_co_cache;

#if __TBB_ENQUEUE_ENFORCED_CONCURRENCY
    // arena needs an extra worker despite the arena limit
    atomic_flag my_local_concurrency_flag;
    // the number of local mandatory concurrency requests
    int my_local_concurrency_requests;
#endif /* __TBB_ENQUEUE_ENFORCED_CONCURRENCY*/

    //! The number of slots in the arena.
    unsigned my_num_slots;
    //! The number of reserved slots (can be occupied only by external threads).
    unsigned my_num_reserved_slots;
    //! The number of workers requested by the external thread owning the arena.
    unsigned my_max_num_workers;

    threading_control_client my_tc_client;

#if TBB_USE_ASSERT
    //! Used to trap accesses to the object after its destruction.
    std::uintptr_t my_guard;
#endif /* TBB_USE_ASSERT */
}; // struct arena_base

class arena: public padded<arena_base>
{
    friend class waiter_base;
    friend class outermost_worker_waiter;
    friend class sleep_waiter;
    friend class task_dispatcher;
    friend void observe(d1::task_scheduler_observer& tso, bool enable);
    friend struct suspend_point_type;
    friend void resume(suspend_point_type* sp);
    friend task_dispatcher& create_coroutine(thread_data& td);
    friend void spawn(d1::task& t, d1::task_group_context& ctx, d1::slot_id id);
    friend void submit(d1::task& t, d1::task_group_context& ctx, arena* a, std::uintptr_t as_critical);
    friend class governor;
    friend struct task_group_context_impl;
    friend struct task_arena_impl;
    friend class nested_arena_context;
    friend class thread_data;
    friend class delegated_task;
    friend void notify_waiters(std::uintptr_t wait_ctx_addr);

public:
    using base_type = padded<arena_base>;

    //! Types of work advertised by advertise_new_work()
    enum new_work_type {
        work_spawned,
        wakeup,
        work_enqueued
    };

    //! Constructor
    // arena (permit_manager& m, unsigned max_num_workers, unsigned num_reserved_slots, unsigned priority_level);
    arena (threading_control& control, unsigned max_num_workers, unsigned num_reserved_slots, unsigned priority_level);

    //! Allocate an instance of arena.
    static arena& allocate_arena(threading_control& control, unsigned num_slots, unsigned num_reserved_slots,
                                  unsigned priority_level);

    static arena& create(threading_control* control, int num_slots, int num_reserved_slots, unsigned arena_priority_level);

    static int unsigned num_arena_slots ( unsigned num_slots ) {
        return max(2u, num_slots);
    }

    static int allocation_size ( unsigned num_slots ) {
        return sizeof(base_type) + num_slots * (sizeof(mail_outbox) + sizeof(arena_slot) + sizeof(task_dispatcher));
    }

    //! Get reference to mailbox corresponding to given slot_id
    mail_outbox& mailbox( d1::slot_id slot ) {
        __TBB_ASSERT( slot != d1::no_slot, "affinity should be specified" );

        return reinterpret_cast<mail_outbox*>(this)[-(int)(slot+1)]; // cast to 'int' is redundant but left for readability
    }

    //! Completes arena shutdown, destructs and deallocates it.
    void free_arena ();

    //! No tasks to steal since last snapshot was taken
    static const pool_state_t SNAPSHOT_EMPTY = 0;

    //! At least one task has been offered for stealing since the last snapshot started
    static const pool_state_t SNAPSHOT_FULL = pool_state_t(-1);

    //! The number of least significant bits for external references
    static const unsigned ref_external_bits = 12; // up to 4095 external and 1M workers

    //! Reference increment values for externals and workers
    static const unsigned ref_external = 1;
    static const unsigned ref_worker   = 1 << ref_external_bits;

    //! No tasks to steal or snapshot is being taken.
    static bool is_busy_or_empty( pool_state_t s ) { return s < SNAPSHOT_FULL; }

    //! The number of workers active in the arena.
    unsigned num_workers_active() const {
        return my_references.load(std::memory_order_acquire) >> ref_external_bits;
    }

    //! Check if the recall is requested by the market.
    bool is_recall_requested() const {
        return num_workers_active() > my_num_workers_allotted.load(std::memory_order_relaxed);
    }

    void enable_mandatory_concurrency();

    void publish_work(arena::new_work_type work_type);

    //! If necessary, raise a flag that there is new job in arena.
    template<arena::new_work_type work_type> void advertise_new_work();

    //! Attempts to steal a task from a randomly chosen arena slot
    d1::task* steal_task(unsigned arena_index, FastRandom& frnd, execution_data_ext& ed, isolation_type isolation);

    //! Get a task from a global starvation resistant queue
    template<task_stream_accessor_type accessor>
    d1::task* get_stream_task(task_stream<accessor>& stream, unsigned& hint);

#if __TBB_PREVIEW_CRITICAL_TASKS
    //! Tries to find a critical task in global critical task stream
    d1::task* get_critical_task(unsigned& hint, isolation_type isolation);
#endif

    //! Check if there is job anywhere in arena.
    /** Return true if no job or if arena is being cleaned up. */
    bool is_out_of_work();

    //! enqueue a task into starvation-resistance queue
    void enqueue_task(d1::task&, d1::task_group_context&, thread_data&);

    //! Registers the worker with the arena and enters TBB scheduler dispatch loop
    void process(thread_data&);

    //! Notification that the thread leaves its arena

    void on_thread_leaving(unsigned ref_param);

    //! Check for the presence of enqueued tasks at all priority levels
    bool has_enqueued_tasks();

    static const std::size_t out_of_arena = ~size_t(0);
    //! Tries to occupy a slot in the arena. On success, returns the slot index; if no slot is available, returns out_of_arena.
    template <bool as_worker>
    std::size_t occupy_free_slot(thread_data&);
    //! Tries to occupy a slot in the specified range.
    std::size_t occupy_free_slot_in_range(thread_data& tls, std::size_t lower, std::size_t upper);

    std::uintptr_t calculate_stealing_threshold();

    unsigned priority_level() {
        return my_priority_level;
    }

    int num_workers_requested() {
        return my_num_workers_requested;
    }

    unsigned references() {
        return my_references.load(std::memory_order_acquire);
    }

    bool is_top_priority();

    bool try_join() {
        if (num_workers_active() < my_num_workers_allotted.load(std::memory_order_relaxed)) {
            my_references += arena::ref_worker;
            return true;
        }
        return false;
    }

    void set_allotment(unsigned allotment) {
        my_num_workers_allotted.store(allotment, std::memory_order_relaxed);
    }

    int update_request(int delta, bool mandatory) {
#if __TBB_ENQUEUE_ENFORCED_CONCURRENCY
        if (mandatory) {
            __TBB_ASSERT(delta == 1 || delta == -1, nullptr);
            // Count the number of mandatory requests and proceed only for 0->1 and 1->0 transitions.
            my_local_concurrency_requests += delta;
            if ((delta > 0 && my_local_concurrency_requests != 1) ||
                (delta < 0 && my_local_concurrency_requests != 0))
            {
                return 0;
            }
        }
#endif
        my_total_num_workers_requested += delta;
        int target_workers = 0;
        // Cap target_workers into interval [0, a.my_max_num_workers]
        if (my_total_num_workers_requested > 0) {
#if __TBB_ENQUEUE_ENFORCED_CONCURRENCY
            // At least one thread should be requested when mandatory concurrency
            int max_num_workers = int(my_max_num_workers);
            if (my_local_concurrency_requests > 0 && max_num_workers == 0) {
                max_num_workers = 1;
            }
#endif
            target_workers = min(my_total_num_workers_requested, max_num_workers);
        }

        delta = target_workers - my_num_workers_requested;

        if (delta == 0) {
            return 0;
        }

        my_num_workers_requested += delta;
        if (my_num_workers_requested == 0) {
            my_num_workers_allotted.store(0, std::memory_order_relaxed);
        }

        return delta;
    }

    /** Must be the last data field */
    arena_slot my_slots[1];
}; // class arena

template<arena::new_work_type work_type>
void arena::advertise_new_work() {
    if( work_type == work_enqueued ) {
        atomic_fence_seq_cst();
        enable_mandatory_concurrency();
        // Local memory fence here and below is required to avoid missed wakeups; see the comment below.
        // Starvation resistant tasks require concurrency, so missed wakeups are unacceptable.
    }
    else if( work_type == wakeup ) {
        atomic_fence_seq_cst();
    }

    // Double-check idiom that, in case of spawning, is deliberately sloppy about memory fences.
    // Technically, to avoid missed wakeups, there should be a full memory fence between the point we
    // released the task pool (i.e. spawned task) and read the arena's state.  However, adding such a
    // fence might hurt overall performance more than it helps, because the fence would be executed
    // on every task pool release, even when stealing does not occur.  Since TBB allows parallelism,
    // but never promises parallelism, the missed wakeup is not a correctness problem.
    pool_state_t snapshot = my_pool_state.load(std::memory_order_acquire);
    if( is_busy_or_empty(snapshot) ) {
        publish_work(work_type);
    }
}

inline d1::task* arena::steal_task(unsigned arena_index, FastRandom& frnd, execution_data_ext& ed, isolation_type isolation) {
    auto slot_num_limit = my_limit.load(std::memory_order_relaxed);
    if (slot_num_limit == 1) {
        // No slots to steal from
        return nullptr;
    }
    // Try to steal a task from a random victim.
    std::size_t k = frnd.get() % (slot_num_limit - 1);
    // The following condition excludes the external thread that might have
    // already taken our previous place in the arena from the list .
    // of potential victims. But since such a situation can take
    // place only in case of significant oversubscription, keeping
    // the checks simple seems to be preferable to complicating the code.
    if (k >= arena_index) {
        ++k; // Adjusts random distribution to exclude self
    }
    arena_slot* victim = &my_slots[k];
    d1::task **pool = victim->task_pool.load(std::memory_order_relaxed);
    d1::task *t = nullptr;
    if (pool == EmptyTaskPool || !(t = victim->steal_task(*this, isolation, k))) {
        return nullptr;
    }
    if (task_accessor::is_proxy_task(*t)) {
        task_proxy &tp = *(task_proxy*)t;
        d1::slot_id slot = tp.slot;
        t = tp.extract_task<task_proxy::pool_bit>();
        if (!t) {
            // Proxy was empty, so it's our responsibility to free it
            tp.allocator.delete_object(&tp, ed);
            return nullptr;
        }
        // Note affinity is called for any stolen task (proxy or general)
        ed.affinity_slot = slot;
    } else {
        // Note affinity is called for any stolen task (proxy or general)
        ed.affinity_slot = d1::any_slot;
    }
    // Update task owner thread id to identify stealing
    ed.original_slot = k;
    return t;
}

template<task_stream_accessor_type accessor>
inline d1::task* arena::get_stream_task(task_stream<accessor>& stream, unsigned& hint) {
    if (stream.empty())
        return nullptr;
    return stream.pop(subsequent_lane_selector(hint));
}

#if __TBB_PREVIEW_CRITICAL_TASKS
// Retrieves critical task respecting isolation level, if provided. The rule is:
// 1) If no outer critical task and no isolation => take any critical task
// 2) If working on an outer critical task and no isolation => cannot take any critical task
// 3) If no outer critical task but isolated => respect isolation
// 4) If working on an outer critical task and isolated => respect isolation
// Hint is used to keep some LIFO-ness, start search with the lane that was used during push operation.
inline d1::task* arena::get_critical_task(unsigned& hint, isolation_type isolation) {
    if (my_critical_task_stream.empty())
        return nullptr;

    if ( isolation != no_isolation ) {
        return my_critical_task_stream.pop_specific( hint, isolation );
    } else {
        return my_critical_task_stream.pop(preceding_lane_selector(hint));
    }
}
#endif // __TBB_PREVIEW_CRITICAL_TASKS

} // namespace r1
} // namespace detail
} // namespace tbb

#endif /* _TBB_arena_H */
