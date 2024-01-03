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

#ifndef _TBB_arena_fixed_size_H
#define _TBB_arena_fixed_size_H

#include <atomic>
#include <cstring>

#include "oneapi/tbb/detail/_task.h"
#include "oneapi/tbb/detail/_utils.h"
#include "oneapi/tbb/spin_mutex.h"

#include "scheduler_common.h"
#include "intrusive_list.h"
#include "task_stream.h"
#include "arena_slot.h"
#include "rml_tbb.h"
#include "mailbox.h"
#include "governor.h"
#include "concurrent_monitor.h"
#include "observer_proxy.h"
#include "thread_control_monitor.h"
#include "threading_control_client.h"

namespace tbb {
namespace prototype {
namespace r1 {

class task_dispatcher;
class task_group_context;
class threading_control;
class allocate_root_with_context_proxy;

struct stack_anchor_type {
    stack_anchor_type() = default;
    stack_anchor_type(const stack_anchor_type&) = delete;
};


class arena_fixed_size: public tbb::detail::r1::arena
{
public:

    //! Types of work advertised by advertise_new_work()
    enum new_work_type {
        work_spawned,
        wakeup,
        work_enqueued
    };

    //! Constructor
    arena_fixed_size(tbb::detail::r1::threading_control* control, unsigned max_num_workers, unsigned num_reserved_slots, unsigned priority_level);
    //! Allocate an instance of arena.
    static arena_fixed_size& allocate_arena(tbb::detail::r1::threading_control* control, unsigned num_slots, unsigned num_reserved_slots,
        unsigned priority_level);

    static arena_fixed_size& create(tbb::detail::r1::threading_control* control, unsigned num_slots, unsigned num_reserved_slots, unsigned arena_priority_level, tbb::detail::d1::constraints constraints = tbb::detail::d1::constraints{});


    static int unsigned num_arena_slots ( unsigned num_slots, unsigned num_reserved_slots ) {
        return num_reserved_slots == 0 ? num_slots : tbb::detail::r1::max(2u, num_slots);
    }

    static int allocation_size( unsigned num_slots ) {
        return sizeof(base_type) + num_slots * (sizeof(tbb::detail::r1::mail_outbox) + sizeof(tbb::detail::r1::arena_slot) + sizeof(tbb::detail::r1::task_dispatcher));
    }


    //! Completes arena_fixed_size shutdown, destructs and deallocates it.
    void free_arena();

    //! The number of least significant bits for external references
    static const unsigned ref_external_bits = 12; // up to 4095 external and 1M workers

    //! Reference increment values for externals and workers
    static const unsigned ref_external = 1;
    static const unsigned ref_worker   = 1 << ref_external_bits;

    //! The number of workers active in the arena_fixed_size.
    unsigned num_workers_active() const {
        return my_references.load(std::memory_order_acquire) >> ref_external_bits;
    }

    //! Check if the recall is requested by the market.
    bool is_recall_requested() const {
        return num_workers_active() > my_num_workers_allotted.load(std::memory_order_relaxed);
    }

    void request_workers(int mandatory_delta, int workers_delta, bool wakeup_threads = false);

    //! If necessary, raise a flag that there is new job in arena_fixed_size.
    template<arena_fixed_size::new_work_type work_type> void advertise_new_work();

    //! Attempts to steal a task from a randomly chosen arena_fixed_size slot
    tbb::detail::d1::task* steal_task(unsigned arena_index, tbb::detail::r1::FastRandom& frnd, tbb::detail::r1::execution_data_ext& ed, tbb::detail::r1::isolation_type isolation);

    //! Get a task from a global starvation resistant queue
    template<tbb::detail::r1::task_stream_accessor_type accessor>
    tbb::detail::d1::task* get_stream_task(tbb::detail::r1::task_stream<accessor>& stream, unsigned& hint);


    //! Check if there is job anywhere in arena_fixed_size.
    void out_of_work();

    //! Registers the worker with the arena_fixed_size and enters TBB scheduler dispatch loop
    void process(tbb::detail::r1::thread_data&);

    //! Notification that the thread leaves its arena_fixed_size

    void on_thread_leaving(unsigned ref_param);

    //! Check for the presence of enqueued tasks
    bool has_enqueued_tasks();

    //! Check for the presence of any tasks
    bool has_tasks();

    bool is_empty() { return my_pool_state.test() == /* EMPTY */ false; }

    tbb::detail::r1::thread_control_monitor& get_waiting_threads_monitor();

    static const std::size_t out_of_arena = ~size_t(0);
    //! Tries to occupy a slot in the arena_fixed_size. On success, returns the slot index; if no slot is available, returns out_of_arena.
    template <bool as_worker>
    std::size_t occupy_free_slot(tbb::detail::r1::thread_data&);
    //! Tries to occupy a slot in the specified range.
    //std::size_t occupy_free_slot_in_range(tbb::detail::r1::thread_data& tls, std::size_t lower, std::size_t upper);

    //std::uintptr_t calculate_stealing_threshold();

    unsigned priority_level() { return my_priority_level; }

    bool has_request() { return my_total_num_workers_requested; }

    unsigned references() const { return my_references.load(std::memory_order_acquire); }

    bool is_arena_workerless() const { return my_max_num_workers == 0; }

    void set_top_priority(bool);

    bool is_top_priority() const;

    bool try_join();

    void set_allotment(unsigned allotment);

    int update_concurrency(unsigned concurrency);

    std::pair</*min workers = */ int, /*max workers = */ int> update_request(int mandatory_delta, int workers_delta);

    /** Must be the last data field */
    tbb::detail::r1::arena_slot my_slots[1];
}; // class arena_fixed_size

template <arena_fixed_size::new_work_type work_type>
void arena_fixed_size::advertise_new_work() {
    bool is_mandatory_needed = false;
    bool are_workers_needed = false;

    if (work_type != work_spawned) {
        // Local memory fence here and below is required to avoid missed wakeups; see the comment below.
        // Starvation resistant tasks require concurrency, so missed wakeups are unacceptable.
        tbb::detail::d0::atomic_fence_seq_cst();
    }

    if (work_type == work_enqueued && my_num_slots > my_num_reserved_slots) {
        is_mandatory_needed = my_mandatory_concurrency.test_and_set();
    }

    // Double-check idiom that, in case of spawning, is deliberately sloppy about memory fences.
    // Technically, to avoid missed wakeups, there should be a full memory fence between the point we
    // released the task pool (i.e. spawned task) and read the arena_fixed_size's state.  However, adding such a
    // fence might hurt overall performance more than it helps, because the fence would be executed
    // on every task pool release, even when stealing does not occur.  Since TBB allows parallelism,
    // but never promises parallelism, the missed wakeup is not a correctness problem.
    are_workers_needed = my_pool_state.test_and_set();

    if (is_mandatory_needed || are_workers_needed) {
        int mandatory_delta = is_mandatory_needed ? 1 : 0;
        int workers_delta = are_workers_needed ? my_max_num_workers : 0;

        if (is_mandatory_needed && is_arena_workerless()) {
            // Set workers_delta to 1 to keep arena_fixed_size invariants consistent
            workers_delta = 1;
        }

        bool wakeup_workers = is_mandatory_needed || are_workers_needed;
        request_workers(mandatory_delta, workers_delta, wakeup_workers);
    }
}

inline tbb::detail::d1::task* arena_fixed_size::steal_task(unsigned arena_index, tbb::detail::r1::FastRandom& frnd, tbb::detail::r1::execution_data_ext& ed, tbb::detail::r1::isolation_type isolation) {
    auto slot_num_limit = my_limit.load(std::memory_order_relaxed);
    if (slot_num_limit == 1) {
        // No slots to steal from
        return nullptr;
    }
    // Try to steal a task from a random victim.
    std::size_t k = frnd.get() % (slot_num_limit - 1);
    // The following condition excludes the external thread that might have
    // already taken our previous place in the arena_fixed_size from the list .
    // of potential victims. But since such a situation can take
    // place only in case of significant oversubscription, keeping
    // the checks simple seems to be preferable to complicating the code.
    if (k >= arena_index) {
        ++k; // Adjusts random distribution to exclude self
    }
    //tbb::detail::r1::arena_slot* victim = &my_slots[k];
    //tbb::detail::d1::task **pool = victim->task_pool.load(std::memory_order_relaxed);
    tbb::detail::d1::task *t = nullptr;
#if 0 // TODO : cannot call steal_task since it expect arena base class type
    if (pool == tbb::detail::r1::EmptyTaskPool || !(t = victim->steal_task(*this, isolation, k))) {
        return nullptr;
    }
#endif
    if (tbb::detail::r1::task_accessor::is_proxy_task(*t)) {
        tbb::detail::r1::task_proxy &tp = *(tbb::detail::r1::task_proxy*)t;
        tbb::detail::d1::slot_id slot = tp.slot;
        t = tp.extract_task<tbb::detail::r1::task_proxy::pool_bit>();
        if (!t) {
            // Proxy was empty, so it's our responsibility to free it
            tp.allocator.delete_object(&tp, ed);
            return nullptr;
        }
        // Note affinity is called for any stolen task (proxy or general)
        ed.affinity_slot = slot;
    } else {
        // Note affinity is called for any stolen task (proxy or general)
        ed.affinity_slot = tbb::detail::d1::any_slot;
    }
    // Update task owner thread id to identify stealing
    ed.original_slot = k;
    return t;
}

template<tbb::detail::r1::task_stream_accessor_type accessor>
inline tbb::detail::d1::task* arena_fixed_size::get_stream_task(tbb::detail::r1::task_stream<accessor>& stream, unsigned& hint) {
    if (stream.empty())
        return nullptr;
    return stream.pop(tbb::detail::r1::subsequent_lane_selector(hint));
}

} //namespace r1
} // namespace prototype
} // namespace tbb

#endif /* _TBB_arena_fixed_size_H */
