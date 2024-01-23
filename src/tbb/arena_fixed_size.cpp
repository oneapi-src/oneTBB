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


arena_fixed_size::arena_fixed_size(tbb::detail::r1::threading_control* control, unsigned num_slots, unsigned num_reserved_slots, unsigned priority_level) :
    arena(control, num_slots, num_reserved_slots, priority_level)  {

}

arena_fixed_size& arena_fixed_size::allocate_arena(tbb::detail::r1::threading_control* control, unsigned num_slots, unsigned num_reserved_slots,
    unsigned priority_level)
{
    __TBB_ASSERT(sizeof(base_type) + sizeof(tbb::detail::r1::arena_slot) == sizeof(arena_fixed_size), "All arena data fields must go to arena_base");
    __TBB_ASSERT(sizeof(base_type) % tbb::detail::r1::cache_line_size() == 0, "arena slots area misaligned: wrong padding");
    //__TBB_ASSERT(sizeof(tbb::detail::r1::mail_outbox) == tbb::detail::d1::max_nfs_size, "Mailbox padding is wrong");
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
    static void execute(d1::task_arena_fixed_size_base&, tbb::detail::d1::delegate_base&);
};
void __TBB_EXPORTED_FUNC initialize(d1::task_arena_fixed_size_base& ta) {
    task_arena_fixed_size_impl::initialize(ta);
}
void __TBB_EXPORTED_FUNC terminate(d1::task_arena_fixed_size_base& ta) {
    task_arena_fixed_size_impl::terminate(ta);
}
void __TBB_EXPORTED_FUNC execute(d1::task_arena_fixed_size_base& ta, tbb::detail::d1::delegate_base& d) {
    task_arena_fixed_size_impl::execute(ta, d);
}

void task_arena_fixed_size_impl::execute(d1::task_arena_fixed_size_base& ta, tbb::detail::d1::delegate_base& d) {
    arena_fixed_size* a = ta.my_arena.load(std::memory_order_relaxed);
    __TBB_ASSERT(a != nullptr, nullptr);
    tbb::detail::r1::thread_data* td = tbb::detail::r1::governor::get_thread_data();

    bool same_arena = td->my_arena == a;
    std::size_t index1 = td->my_arena_index;
    if (!same_arena) {
        index1 = a->occupy_free_slot</*as_worker */false>(*td);
        if (index1 == arena_fixed_size::out_of_arena) {
            tbb::detail::r1::concurrent_monitor::thread_context waiter((std::uintptr_t)&d);
            tbb::detail::d1::wait_context wo(1);
            tbb::detail::d1::task_group_context exec_context(tbb::detail::d1::task_group_context::isolated);
            tbb::detail::r1::task_group_context_impl::copy_fp_settings(exec_context, *a->my_default_ctx);

            //tbb::detail::r1::delegated_task dt(d, a->my_exit_monitors, wo);
            //a->enqueue_task( dt, exec_context, *td);
            // process possible exception
            return;
        } // if (index1 == arena::out_of_arena)
    } // if (!same_arena)

}


void task_arena_fixed_size_impl::initialize(d1::task_arena_fixed_size_base& ta) {
    // Enforce global market initialization to properly initialize soft limit
    (void)tbb::detail::r1::governor::get_thread_data();
    tbb::detail::d1::constraints arena_constraints;

    __TBB_ASSERT(ta.my_arena.load(std::memory_order_relaxed) == nullptr, "Arena already initialized");
    unsigned priority_level = 1;
    tbb::detail::r1::threading_control* thr_control = tbb::detail::r1::threading_control::register_public_reference();
    arena_fixed_size& a = arena_fixed_size::create(thr_control, unsigned(ta.my_max_concurrency), ta.my_num_reserved_slots, priority_level, arena_constraints);
    //arena& a = arena::create(thr_control, unsigned(ta.my_max_concurrency), ta.my_num_reserved_slots, priority_level, arena_constraints);

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

