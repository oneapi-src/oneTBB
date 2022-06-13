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

#include "arena.h"
#include "market.h"

#include <algorithm>

namespace tbb {
namespace detail {
namespace r1 {


class tbb_permit_manager_client : public pm_client {
public:
    tbb_permit_manager_client(arena& a) : pm_client(a) {}

    void set_allotment(unsigned allotment) {
        my_arena.set_allotment(allotment);
    }

    //! The index in the array of per priority lists of arenas this object is in.
    unsigned priority_level() {
        return my_arena.priority_level();
    }

    void set_top_priority(bool b) {
        return my_is_top_priority.store(b, std::memory_order_relaxed);
    }

    void register_thread() override {}
    void unregister_thread() override {}
};

//------------------------------------------------------------------------
// market
//------------------------------------------------------------------------

market::market(unsigned workers_soft_limit)
    : my_num_workers_soft_limit(workers_soft_limit)
{}

pm_client* market::create_client(arena& a, constraints_type*) {
    return new (cache_aligned_allocate(sizeof(tbb_permit_manager_client))) tbb_permit_manager_client(a);
}

void market::register_client(pm_client* c) {
    mutex_type::scoped_lock lock(my_mutex);
    auto client = static_cast<tbb_permit_manager_client*>(c);
    my_clients[client->priority_level()].push_back(client);
}

void market::destroy_client(pm_client& c) {
    auto client = static_cast<tbb_permit_manager_client*>(&c);
    {
        mutex_type::scoped_lock lock(my_mutex);
        auto& clients = my_clients[client->priority_level()];
        auto it = std::find(clients.begin(), clients.end(), client);
        clients.erase(it);
    }

    client->~tbb_permit_manager_client();
    cache_aligned_deallocate(client);
}

void market::update_allotment() {
    int effective_soft_limit = my_mandatory_num_requested > 0 && my_num_workers_soft_limit == 0 ? 1 : my_num_workers_soft_limit;
    int max_workers = min(my_total_demand, effective_soft_limit);
    __TBB_ASSERT(max_workers >= 0, nullptr);

    int unassigned_workers = max_workers;
    int assigned = 0;
    int carry = 0;
    unsigned max_priority_level = num_priority_levels;
    for (unsigned list_idx = 0; list_idx < num_priority_levels; ++list_idx ) {
        int assigned_per_priority = min(my_priority_level_demand[list_idx], unassigned_workers);
        unassigned_workers -= assigned_per_priority;
        for (auto it = my_clients[list_idx].rbegin(); it != my_clients[list_idx].rend(); ++it) {
            tbb_permit_manager_client& client = **it;
            if (client.max_workers() == 0) {
                client.set_allotment(0);
                continue;
            }

            if (max_priority_level == num_priority_levels) {
                max_priority_level = list_idx;
            }

            int allotted = 0;
            if (my_num_workers_soft_limit == 0) {
                __TBB_ASSERT(max_workers == 0 || max_workers == 1, nullptr);
                allotted = client.min_workers() > 0 && assigned < max_workers ? 1 : 0;
            } else {
                int tmp = client.max_workers() * assigned_per_priority + carry;
                allotted = tmp / my_priority_level_demand[list_idx];
                carry = tmp % my_priority_level_demand[list_idx];
                __TBB_ASSERT(allotted <= client.max_workers(), nullptr);
            }
            client.set_allotment(allotted);
            client.set_top_priority(list_idx == max_priority_level);
            assigned += allotted;
        }
    }
    __TBB_ASSERT(assigned == max_workers, nullptr);
}

void market::set_active_num_workers(int soft_limit) {
    mutex_type::scoped_lock lock(my_mutex);
    if (my_num_workers_soft_limit != soft_limit) {
        my_num_workers_soft_limit = soft_limit;
        update_allotment();
    }
}

void market::adjust_demand(pm_client& c, int mandatory_delta, int workers_delta) {
    __TBB_ASSERT(-1 <= mandatory_delta && mandatory_delta <= 1, nullptr);

    auto& client = static_cast<tbb_permit_manager_client&>(c);
    int delta{};
    {
        mutex_type::scoped_lock lock(my_mutex);
        // Update client's state
        delta = client.update_request(mandatory_delta, workers_delta);

        // Update makret's state
        my_total_demand += delta;
        my_priority_level_demand[client.priority_level()] += delta;
        my_mandatory_num_requested += mandatory_delta;

        update_allotment();
    }

    notify_thread_request(delta);
}

} // namespace r1
} // namespace detail
} // namespace tbb
