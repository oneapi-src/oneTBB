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

#ifndef _TBB_clients_H
#define _TBB_clients_H

#include "oneapi/tbb/detail/_intrusive_list_node.h"
#include "arena.h"

namespace tbb {
namespace detail {
namespace r1 {

class arena;

using mask_type = void*;

class thread_pool_ticket : public d1::intrusive_list_node /* Need for list in thread pool */ {
public:
    thread_pool_ticket(arena& a) : my_arena(a) {}
    void apply_mask(mask_type mask) { suppress_unused_warning(mask); }
    // Interface of communication with thread pool
    bool try_join() {
        return my_arena.try_join();
    }
    void process(thread_data& td) {
        my_arena.process(td);
    }

    unsigned priority_level() {
        return my_arena.priority_level();
    }
private:
    arena& my_arena;
    // mask_type m_mask;
};

class thread_dispatcher;

// TODO resource_manager_client and thread_pool_client
class permit_manager_client {
public:
    permit_manager_client(arena& a, permit_manager& pm, thread_dispatcher& td)
        : my_arena(a)
        , my_permit_manager(pm)
        , my_thread_dispatcher(td)
        , my_ticket(a)
    {}

    virtual ~permit_manager_client() {}

    virtual void request_demand(unsigned min, unsigned max) = 0;
    virtual void release_demand() = 0;

    bool is_top_priority() {
        return my_is_top_priority.load(std::memory_order_relaxed);
    }

    int update_request(int delta, bool mandatory) {
        return my_arena.update_request(delta, mandatory);
    }

    //! The target serialization epoch for callers of adjust_job_count_estimate
    int my_adjust_demand_target_epoch{ 0 };

    //! The current serialization epoch for callers of adjust_job_count_estimate
    d1::waitable_atomic<int> my_adjust_demand_current_epoch{ 0 };

protected:
    arena& my_arena;
    permit_manager& my_permit_manager;
    thread_dispatcher& my_thread_dispatcher;
    thread_pool_ticket my_ticket;
    //! The max priority level of arena in market.
    std::atomic<bool> my_is_top_priority{ false };
};


} // namespace r1
} // namespace detail
} // namespace tbb



#endif // _TBB_clients_H
