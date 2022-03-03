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

#ifndef _TBB_market_H
#define _TBB_market_H

#include "scheduler_common.h"
#include "market_concurrent_monitor.h"
#include "intrusive_list.h"
#include "oneapi/tbb/rw_mutex.h"

#include "oneapi/tbb/spin_rw_mutex.h"
#include "oneapi/tbb/task_group.h"
#include "resource_manager.h"

#include <atomic>

#if defined(_MSC_VER) && defined(_Wp64)
    // Workaround for overzealous compiler warnings in /Wp64 mode
    #pragma warning (push)
    #pragma warning (disable: 4244)
#endif

namespace tbb {
namespace detail {

namespace d1 {
class task_scheduler_handle;
}

namespace r1 {

class permit_manager_client;
struct tbb_permit_manager_client;
class task_group_context;
class thread_dispatcher;


//------------------------------------------------------------------------
// Class market
//------------------------------------------------------------------------

class market : public permit_manager {
    friend class task_group_context;
    friend class governor;
    friend class lifetime_control;

public:
    //! Keys for the arena map array. The lower the value the higher priority of the arena list.
    static constexpr unsigned num_priority_levels = 3;

private:
    friend void ITT_DoUnsafeOneTimeInitialization ();
    friend bool finalize_impl(d1::task_scheduler_handle& handle);
    friend thread_dispatcher;

    using arena_list_type = intrusive_list<tbb_permit_manager_client>;
    using arenas_list_mutex_type = d1::rw_mutex;

    // TODO: introduce fine-grained (per priority list) locking of arenas.
    arenas_list_mutex_type my_arenas_list_mutex;

    //! Current application-imposed limit on the number of workers (see set_active_num_workers())
    /** It can't be more than my_num_workers_hard_limit. **/
    std::atomic<unsigned> my_num_workers_soft_limit;

    //! Number of workers currently requested from RML
    int my_num_workers_requested;

    //! Number of workers that were requested by all arenas on all priority levels
    std::atomic<int> my_total_demand;

    //! Number of workers that were requested by arenas per single priority list item
    int my_priority_level_demand[num_priority_levels];

#if __TBB_ENQUEUE_ENFORCED_CONCURRENCY
    //! How many times mandatory concurrency was requested from the market
    int my_mandatory_num_requested;
#endif

    //! Per priority list of registered arenas
    arena_list_type my_arenas[num_priority_levels];

    //! The first arena to be checked when idle worker seeks for an arena to enter
    /** The check happens in round-robin fashion. **/
    tbb_permit_manager_client* my_next_arena;

    //! ABA prevention marker to assign to newly created arenas
    std::atomic<uintptr_t> my_arenas_aba_epoch;

    //! The value indicating that the soft limit warning is unnecessary
    static const unsigned skip_soft_limit_warning = ~0U;

    //! Either workers soft limit to be reported via runtime_warning() or skip_soft_limit_warning
    std::atomic<unsigned> my_workers_soft_limit_to_report;

    //! Recalculates the number of workers requested from RML and updates the allotment.
    int update_workers_request();

    //! Recalculates the number of workers assigned to each arena in the list.
    /** The actual number of workers servicing a particular arena may temporarily
        deviate from the calculated value. **/
    void update_allotment (unsigned effective_soft_limit) {
        int total_demand = my_total_demand.load(std::memory_order_relaxed);
        if (total_demand) {
            update_allotment(my_arenas, total_demand, (int)effective_soft_limit);
        }
    }

    tbb_permit_manager_client* select_next_arena(tbb_permit_manager_client* hint );

    void insert_arena_into_list (tbb_permit_manager_client& a );

    void remove_arena_from_list (tbb_permit_manager_client& a );

    int update_allotment ( arena_list_type* arenas, int total_demand, int max_workers );

public:
    //! Constructor
    market(unsigned workers_soft_limit);

    //! Destructor
    ~market();

    permit_manager_client* create_client(arena& a, constraits_type* constraits) override;
    void destroy_client(permit_manager_client& c) override;

    //! Removes the arena from the market's list
    bool try_destroy_arena (permit_manager_client*, uintptr_t aba_epoch, unsigned priority_level ) override;

    //! Removes the arena from the market's list
    void detach_arena (tbb_permit_manager_client& );

#if __TBB_ENQUEUE_ENFORCED_CONCURRENCY
    //! Imlpementation of mandatory concurrency enabling
    void enable_mandatory_concurrency_impl (tbb_permit_manager_client*a );

    bool is_global_concurrency_disabled(permit_manager_client*c );

    //! Inform the external thread that there is an arena with mandatory concurrency
    int enable_mandatory_concurrency(permit_manager_client*c ) override;

    //! Inform the external thread that the arena is no more interested in mandatory concurrency
    void disable_mandatory_concurrency_impl(tbb_permit_manager_client* a);

    //! Inform the external thread that the arena is no more interested in mandatory concurrency
    int mandatory_concurrency_disable(permit_manager_client*c ) override;
#endif /* __TBB_ENQUEUE_ENFORCED_CONCURRENCY */

    //! Request that arena's need in workers should be adjusted.
    /** Concurrent invocations are possible only on behalf of different arenas. **/
    std::pair<int, std::int64_t> adjust_demand (permit_manager_client&, int delta, bool mandatory ) override;

    //! Set number of active workers
    int set_active_num_workers(unsigned w) override;

    uintptr_t aba_epoch() override {
        return my_arenas_aba_epoch.load(std::memory_order_relaxed);
    }
}; // class market

} // namespace r1
} // namespace detail
} // namespace tbb

#if defined(_MSC_VER) && defined(_Wp64)
    // Workaround for overzealous compiler warnings in /Wp64 mode
    #pragma warning (pop)
#endif // warning 4244 is back

#endif /* _TBB_market_H */
