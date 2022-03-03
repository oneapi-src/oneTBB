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

#include "oneapi/tbb/global_control.h" // global_control::active_value

#include "thread_dispatcher.h"
#include "market.h"
#include "main.h"
#include "governor.h"
#include "arena.h"
#include "thread_data.h"
#include "itt_notify.h"

#include <cstring> // std::memset()

#include "clients.h"

namespace tbb {
namespace detail {
namespace r1 {


struct tbb_permit_manager_client : public permit_manager_client, public d1::intrusive_list_node {
    tbb_permit_manager_client(arena& a) : permit_manager_client(a) {}

    void request_demand(unsigned min, unsigned max) override {
        suppress_unused_warning(min, max);
    }
    void release_demand() override {}

    void set_allotment(unsigned allotment) {
        my_arena.set_allotment(allotment);
    }

    // arena needs an extra worker despite a global limit
    std::atomic<bool> m_global_concurrency_mode{false};

    //! The index in the array of per priority lists of arenas this object is in.
    unsigned priority_level() {
        return my_arena.priority_level();
    }

    bool has_enqueued_tasks() {
        return my_arena.has_enqueued_tasks();
    }

    std::uintptr_t aba_epoch() {
        return my_arena.aba_epoch();
    }

    int num_workers_requested() {
        return my_arena.num_workers_requested();
    }

    unsigned references() {
        return my_arena.references();
    }

    thread_pool_ticket& ticket() {
        return my_ticket;
    }

    void set_top_priority(bool b) {
        return my_is_top_priority.store(b, std::memory_order_relaxed);
    }
};

void market::insert_arena_into_list (tbb_permit_manager_client& a ) {
    __TBB_ASSERT( a.priority_level() < num_priority_levels, nullptr );
    my_arenas[a.priority_level()].push_front( a );
}

void market::remove_arena_from_list (tbb_permit_manager_client& a ) {
    __TBB_ASSERT( a.priority_level() < num_priority_levels, nullptr );
    my_arenas[a.priority_level()].remove( a );
}

//------------------------------------------------------------------------
// market
//------------------------------------------------------------------------

market::market(unsigned workers_soft_limit )
    : my_num_workers_soft_limit(workers_soft_limit)
    , my_next_arena(nullptr)
    , my_workers_soft_limit_to_report(workers_soft_limit)
{}

market::~market() {
    poison_pointer(my_next_arena);
}

int market::update_workers_request() {
    int old_request = my_num_workers_requested;
    my_num_workers_requested = min(my_total_demand.load(std::memory_order_relaxed),
                                   (int)my_num_workers_soft_limit.load(std::memory_order_relaxed));
#if __TBB_ENQUEUE_ENFORCED_CONCURRENCY
    if (my_mandatory_num_requested > 0) {
        __TBB_ASSERT(my_num_workers_soft_limit.load(std::memory_order_relaxed) == 0, nullptr);
        my_num_workers_requested = 1;
    }
#endif
    update_allotment(my_num_workers_requested);
    return my_num_workers_requested - old_request;
}

int market::set_active_num_workers(unsigned soft_limit) {
    if (my_num_workers_soft_limit.load(std::memory_order_relaxed) == soft_limit) {
        return 0;
    }

    arenas_list_mutex_type::scoped_lock lock(my_arenas_list_mutex);
#if __TBB_ENQUEUE_ENFORCED_CONCURRENCY
    arena_list_type* arenas = my_arenas;

    if (my_num_workers_soft_limit.load(std::memory_order_relaxed) == 0 && my_mandatory_num_requested > 0) {
        for (unsigned level = 0; level < num_priority_levels; ++level ) {
            for (arena_list_type::iterator it = arenas[level].begin(); it != arenas[level].end(); ++it) {
                if (it->m_global_concurrency_mode.load(std::memory_order_relaxed)) {
                    disable_mandatory_concurrency_impl(&*it);
                }
            }
        }
    }
    __TBB_ASSERT(my_mandatory_num_requested == 0, nullptr);
#endif
    my_num_workers_soft_limit.store(soft_limit, std::memory_order_release);
    // report only once after new soft limit value is set
    my_workers_soft_limit_to_report.store(soft_limit, std::memory_order_relaxed);

#if __TBB_ENQUEUE_ENFORCED_CONCURRENCY
    if (my_num_workers_soft_limit.load(std::memory_order_relaxed) == 0) {
        for (unsigned level = 0; level < num_priority_levels; ++level) {
            for (arena_list_type::iterator it = arenas[level].begin(); it != arenas[level].end(); ++it) {
                if (it->has_enqueued_tasks()) {
                    enable_mandatory_concurrency_impl(&*it);
                }
            }
        }
    }
#endif

    return update_workers_request();
}

permit_manager_client* market::create_client(arena& a, constraits_type*) {
    auto c = new tbb_permit_manager_client(a);
    // Add newly created arena into the existing market's list.
    arenas_list_mutex_type::scoped_lock lock(my_arenas_list_mutex);
    insert_arena_into_list(*c);
    return c;
}

void market::destroy_client(permit_manager_client& c) {
    delete &c;
}

/** This method must be invoked under my_arenas_list_mutex. **/
void market::detach_arena(tbb_permit_manager_client& a ) {
    //__TBB_ASSERT( !a.my_slots[0].is_occupied(), NULL );
    if (a.m_global_concurrency_mode.load(std::memory_order_relaxed))
        disable_mandatory_concurrency_impl(&a);

    remove_arena_from_list(a);
    if (a.aba_epoch() == my_arenas_aba_epoch.load(std::memory_order_relaxed)) {
        my_arenas_aba_epoch.store(my_arenas_aba_epoch.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
    }
}

bool market::try_destroy_arena (permit_manager_client* c, uintptr_t aba_epoch, unsigned priority_level ) {
    auto a = static_cast<tbb_permit_manager_client*>(c);
    bool locked = true;
    __TBB_ASSERT( a, nullptr);
    // we hold reference to the server, so market cannot be destroyed at any moment here
    __TBB_ASSERT(!is_poisoned(my_next_arena), nullptr);
    my_arenas_list_mutex.lock();
        arena_list_type::iterator it = my_arenas[priority_level].begin();
        for ( ; it != my_arenas[priority_level].end(); ++it ) {
            if ( a == &*it ) {
                if ( it->aba_epoch() == aba_epoch ) {
                    // Arena is alive
                    // Acquire my_references to sync with threads that just left the arena
                    if (!a->num_workers_requested() && !a->references()) {
                        /*__TBB_ASSERT(
                            !a->my_num_workers_allotted.load(std::memory_order_relaxed) &&
                            (a->my_pool_state == arena::SNAPSHOT_EMPTY || !a->my_max_num_workers),
                            "Inconsistent arena state"
                        );*/
                        // Arena is abandoned. Destroy it.
                        detach_arena( *a );
                        my_arenas_list_mutex.unlock();
                        locked = false;
                        //a->free_arena();
                        return true;
                    }
                }
                if (locked)
                    my_arenas_list_mutex.unlock();
                return false;
            }
        }
    my_arenas_list_mutex.unlock();
    return false;
}

int market::update_allotment ( arena_list_type* arenas, int workers_demand, int max_workers ) {
    __TBB_ASSERT( workers_demand > 0, nullptr );
    max_workers = min(workers_demand, max_workers);
    int unassigned_workers = max_workers;
    int assigned = 0;
    int carry = 0;
    unsigned max_priority_level = num_priority_levels;
    for (unsigned list_idx = 0; list_idx < num_priority_levels; ++list_idx ) {
        int assigned_per_priority = min(my_priority_level_demand[list_idx], unassigned_workers);
        unassigned_workers -= assigned_per_priority;
        for (arena_list_type::iterator it = arenas[list_idx].begin(); it != arenas[list_idx].end(); ++it) {
            tbb_permit_manager_client& a = *it;
            __TBB_ASSERT(a.num_workers_requested() >= 0, nullptr);
            //__TBB_ASSERT(a.num_workers_requested() <= int(a.my_max_num_workers)
            //    || (a.my_max_num_workers == 0 && a.my_local_concurrency_requests > 0 && a.num_workers_requested() == 1), nullptr);
            if (a.num_workers_requested() == 0) {
             //   __TBB_ASSERT(!a.my_num_workers_allotted.load(std::memory_order_relaxed), nullptr);
                continue;
            }

            if (max_priority_level == num_priority_levels) {
                max_priority_level = list_idx;
            }

            int allotted = 0;
#if __TBB_ENQUEUE_ENFORCED_CONCURRENCY
            if (my_num_workers_soft_limit.load(std::memory_order_relaxed) == 0) {
                __TBB_ASSERT(max_workers == 0 || max_workers == 1, nullptr);
                allotted = a.m_global_concurrency_mode.load(std::memory_order_relaxed) &&
                    assigned < max_workers ? 1 : 0;
            } else
#endif
            {
                int tmp = a.num_workers_requested() * assigned_per_priority + carry;
                allotted = tmp / my_priority_level_demand[list_idx];
                carry = tmp % my_priority_level_demand[list_idx];
                __TBB_ASSERT(allotted <= a.num_workers_requested(), nullptr);
                //__TBB_ASSERT(allotted <= int(a.my_num_slots - a.my_num_reserved_slots), nullptr);
            }
            //a.my_num_workers_allotted.store(allotted, std::memory_order_relaxed);
            a.set_allotment(allotted);
            a.set_top_priority(list_idx == max_priority_level);
            assigned += allotted;
        }
    }
    __TBB_ASSERT( 0 <= assigned && assigned <= max_workers, nullptr );
    return assigned;
}

#if __TBB_ENQUEUE_ENFORCED_CONCURRENCY
void market::enable_mandatory_concurrency_impl (tbb_permit_manager_client*a ) {
    __TBB_ASSERT(!a->m_global_concurrency_mode.load(std::memory_order_relaxed), NULL);
    __TBB_ASSERT(my_num_workers_soft_limit.load(std::memory_order_relaxed) == 0, NULL);

    a->m_global_concurrency_mode.store(true, std::memory_order_relaxed);
    my_mandatory_num_requested++;
}

bool market::is_global_concurrency_disabled(permit_manager_client *c) {
    tbb_permit_manager_client* a = static_cast<tbb_permit_manager_client*>(c);
    return my_num_workers_soft_limit.load(std::memory_order_acquire) == 0 && a->m_global_concurrency_mode.load(std::memory_order_acquire) == false;
}

int market::enable_mandatory_concurrency(permit_manager_client *c) {
    tbb_permit_manager_client* a = static_cast<tbb_permit_manager_client*>(c);
    int delta = 0;
    if (is_global_concurrency_disabled(a)) {
        arenas_list_mutex_type::scoped_lock lock(my_arenas_list_mutex);
        if (my_num_workers_soft_limit.load(std::memory_order_relaxed) != 0 || a->m_global_concurrency_mode.load(std::memory_order_relaxed)) {
            return 0;
        }

        enable_mandatory_concurrency_impl(a);
        delta = update_workers_request();
    }
    return delta;
}

void market::disable_mandatory_concurrency_impl(tbb_permit_manager_client* a) {
    __TBB_ASSERT(a->m_global_concurrency_mode.load(std::memory_order_relaxed), NULL);
    __TBB_ASSERT(my_mandatory_num_requested > 0, NULL);

    a->m_global_concurrency_mode.store(false, std::memory_order_relaxed);
    my_mandatory_num_requested--;
}

int market::mandatory_concurrency_disable(permit_manager_client* c) {
    tbb_permit_manager_client* a = static_cast<tbb_permit_manager_client*>(c);
    int delta = 0;
    if ( a->m_global_concurrency_mode.load(std::memory_order_acquire) == true ) {
        arenas_list_mutex_type::scoped_lock lock(my_arenas_list_mutex);
        if (!a->m_global_concurrency_mode.load(std::memory_order_relaxed)) {
            return 0;
        }
        // There is a racy window in advertise_new_work between mandtory concurrency enabling and 
        // setting SNAPSHOT_FULL. It gives a chance to spawn request to disable mandatory concurrency.
        // Therefore, we double check that there is no enqueued tasks.
        if (a->has_enqueued_tasks()) {
            return 0;
        }

        __TBB_ASSERT(my_num_workers_soft_limit.load(std::memory_order_relaxed) == 0, NULL);
        disable_mandatory_concurrency_impl(a);

        delta = update_workers_request();
    }
    return delta;
}
#endif /* __TBB_ENQUEUE_ENFORCED_CONCURRENCY */

std::pair<int, std::int64_t> market::adjust_demand(permit_manager_client& c, int delta, bool mandatory ) {
    auto& a = static_cast<tbb_permit_manager_client&>(c);
    if (!delta) {
        return std::make_pair(0, -1);
    }
    int target_epoch{};
    {
        arenas_list_mutex_type::scoped_lock lock(my_arenas_list_mutex);

        delta = a.update_request(delta, mandatory);

        if (!delta) {
            return std::make_pair(0, -1);
        }

        int total_demand = my_total_demand.load(std::memory_order_relaxed) + delta;
        my_total_demand.store(total_demand, std::memory_order_relaxed);
        my_priority_level_demand[a.priority_level()] += delta;
        unsigned effective_soft_limit = my_num_workers_soft_limit.load(std::memory_order_relaxed);
        if (my_mandatory_num_requested > 0) {
            __TBB_ASSERT(effective_soft_limit == 0, nullptr);
            effective_soft_limit = 1;
        }

        update_allotment(effective_soft_limit);
        if (delta > 0) {
            // can't overflow soft_limit, but remember values request by arenas in
            // my_total_demand to not prematurely release workers to RML
            if (my_num_workers_requested + delta > (int)effective_soft_limit)
                delta = effective_soft_limit - my_num_workers_requested;
        }
        else {
            // the number of workers should not be decreased below my_total_demand
            if (my_num_workers_requested + delta < total_demand)
                delta = min(total_demand, (int)effective_soft_limit) - my_num_workers_requested;
        }
        my_num_workers_requested += delta;
        __TBB_ASSERT(my_num_workers_requested <= (int)effective_soft_limit, nullptr);

        target_epoch = a.my_adjust_demand_target_epoch++;
    }

    return std::make_pair(delta, target_epoch);
}

} // namespace r1
} // namespace detail
} // namespace tbb
