/*
    Copyright 2005-2015 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks. Threading Building Blocks is free software;
    you can redistribute it and/or modify it under the terms of the GNU General Public License
    version 2  as  published  by  the  Free Software Foundation.  Threading Building Blocks is
    distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See  the GNU General Public License for more details.   You should have received a copy of
    the  GNU General Public License along with Threading Building Blocks; if not, write to the
    Free Software Foundation, Inc.,  51 Franklin St,  Fifth Floor,  Boston,  MA 02110-1301 USA

    As a special exception,  you may use this file  as part of a free software library without
    restriction.  Specifically,  if other files instantiate templates  or use macros or inline
    functions from this file, or you compile this file and link it with other files to produce
    an executable,  this file does not by itself cause the resulting executable to be covered
    by the GNU General Public License. This exception does not however invalidate any other
    reasons why the executable file might be covered by the GNU General Public License.
*/

#include "tbb/tbb_stddef.h"
#include "tbb/global_control.h" // global_control::active_value

#include "market.h"
#include "tbb_main.h"
#include "governor.h"
#include "scheduler.h"
#include "itt_notify.h"

namespace tbb {
namespace internal {

void market::insert_arena_into_list ( arena& a ) {
#if __TBB_TASK_PRIORITY
    arena_list_type &arenas = my_priority_levels[a.my_top_priority].arenas;
    arena *&next = my_priority_levels[a.my_top_priority].next_arena;
#else /* !__TBB_TASK_PRIORITY */
    arena_list_type &arenas = my_arenas;
    arena *&next = my_next_arena;
#endif /* !__TBB_TASK_PRIORITY */
    arenas.push_front( a );
    if ( arenas.size() == 1 )
        next = &*arenas.begin();
}

void market::remove_arena_from_list ( arena& a ) {
#if __TBB_TASK_PRIORITY
    arena_list_type &arenas = my_priority_levels[a.my_top_priority].arenas;
    arena *&next = my_priority_levels[a.my_top_priority].next_arena;
#else /* !__TBB_TASK_PRIORITY */
    arena_list_type &arenas = my_arenas;
    arena *&next = my_next_arena;
#endif /* !__TBB_TASK_PRIORITY */
    arena_list_type::iterator it = next;
    __TBB_ASSERT( it != arenas.end(), NULL );
    if ( next == &a ) {
        if ( ++it == arenas.end() && arenas.size() > 1 )
            it = arenas.begin();
        next = &*it;
    }
    arenas.remove( a );
}

//------------------------------------------------------------------------
// market
//------------------------------------------------------------------------

market::market ( unsigned workers_soft_limit, unsigned workers_hard_limit, size_t stack_size )
    : my_num_workers_hard_limit(workers_hard_limit)
    , my_num_workers_soft_limit(workers_soft_limit)
#if __TBB_TASK_PRIORITY
    , my_global_top_priority(normalized_normal_priority)
    , my_global_bottom_priority(normalized_normal_priority)
#endif /* __TBB_TASK_PRIORITY */
    , my_ref_count(1)
    , my_stack_size(stack_size)
{
#if __TBB_TASK_PRIORITY
    __TBB_ASSERT( my_global_reload_epoch == 0, NULL );
    my_priority_levels[normalized_normal_priority].workers_available = my_num_workers_soft_limit;
#endif /* __TBB_TASK_PRIORITY */

    // Once created RML server will start initializing workers that will need 
    // global market instance to get worker stack size
    my_server = governor::create_rml_server( *this );
    __TBB_ASSERT( my_server, "Failed to create RML server" );
}

static unsigned calc_workers_soft_limit(unsigned workers_soft_limit, unsigned workers_hard_limit) {
    if( int soft_limit = market::app_parallelism_limit() )
        workers_soft_limit = soft_limit-1;
    else // if user set no limits (yet), use market's parameter
        workers_soft_limit = max( governor::default_num_threads() - 1, workers_soft_limit );
    // at least 1 worker is required to support starvation resistant tasks
    if( workers_soft_limit == 0 )
        workers_soft_limit = 1;
    else if( workers_soft_limit >= workers_hard_limit )
        workers_soft_limit = workers_hard_limit-1;
    return workers_soft_limit;
}

market& market::global_market ( bool is_public, unsigned workers_soft_limit, size_t stack_size,
                                bool default_concurrency_requested ) {
    global_market_mutex_type::scoped_lock lock( theMarketMutex );
    market *m = theMarket;
    if( m ) {
        ++m->my_ref_count;
        const unsigned old_public_count = is_public? m->my_public_ref_count++ : /*any non-zero value*/1;
        lock.release();
        if( old_public_count==0 ) {
            workers_soft_limit = calc_workers_soft_limit(workers_soft_limit, m->my_num_workers_hard_limit);
            set_active_num_workers( workers_soft_limit );
        }
        if( m->my_num_workers_soft_limit < workers_soft_limit && !default_concurrency_requested )
            runtime_warning( "Max number of workers has been already set to %u. "
                             "The request for %u workers is ignored.\n",
                              m->my_num_workers_soft_limit, workers_soft_limit );
        if( m->my_stack_size < stack_size )
            runtime_warning( "Thread stack size has been already set to %u. "
                             "The request for larger stack (%u) cannot be satisfied.\n",
                              m->my_stack_size, stack_size );
    }
    else {
        // TODO: A lot is done under theMarketMutex locked. Can anything be moved out?
        if( stack_size == 0 )
            stack_size = global_control::active_value(global_control::thread_stack_size);
        // Expecting that 4P is suitable for most applications.
        // Limit to 2P for large thread number.
        // TODO: ask RML for max concurrency and possibly correct hard_limit
        const unsigned factor = governor::default_num_threads()<=128? 4 : 2;
        unsigned workers_hard_limit = max(factor*governor::default_num_threads(), app_parallelism_limit());
        workers_soft_limit = calc_workers_soft_limit(workers_soft_limit, workers_hard_limit);
        // Create the global market instance
        size_t size = sizeof(market);
#if __TBB_TASK_GROUP_CONTEXT
        __TBB_ASSERT( __TBB_offsetof(market, my_workers) + sizeof(generic_scheduler*) == sizeof(market),
                      "my_workers must be the last data field of the market class");
        size += sizeof(generic_scheduler*) * (workers_hard_limit - 1);
#endif /* __TBB_TASK_GROUP_CONTEXT */
        __TBB_InitOnce::add_ref();
        void* storage = NFS_Allocate(1, size, NULL);
        memset( storage, 0, size );
        // Initialize and publish global market
        m = new (storage) market( workers_soft_limit, workers_hard_limit, stack_size );
        if( is_public )
            m->my_public_ref_count = 1;
        theMarket = m;
        // This check relies on the fact that for shared RML default_concurrency==max_concurrency
        if ( !governor::UsePrivateRML && m->my_server->default_concurrency() < workers_soft_limit )
            runtime_warning( "RML might limit the number of workers to %u while %u is requested.\n"
                    , m->my_server->default_concurrency(), workers_soft_limit );
    }
    return *m;
}

void market::destroy () {
#if __TBB_COUNT_TASK_NODES
    if ( my_task_node_count )
        runtime_warning( "Leaked %ld task objects\n", (long)my_task_node_count );
#endif /* __TBB_COUNT_TASK_NODES */
    this->~market();
    NFS_Free( this );
    __TBB_InitOnce::remove_ref();
}

void market::release ( bool is_public ) {
    __TBB_ASSERT( theMarket == this, "Global market instance was destroyed prematurely?" );
    bool do_release = false;
    {
        global_market_mutex_type::scoped_lock lock(theMarketMutex);
        if ( is_public ) {
            __TBB_ASSERT( my_public_ref_count, NULL );
            --my_public_ref_count;
        }
        if ( --my_ref_count == 0 ) {
            do_release = true;
            theMarket = NULL;
        }
    }
    if( do_release )
        my_server->request_close_connection();
}

void market::wait_workers () {
    // usable for this kind of scheduler only
    __TBB_ASSERT(join_workers, NULL);
    // to guarantee that request_close_connection() is called by master,
    // wait till terminating last worker decresed my_ref_count
    while (__TBB_load_with_acquire(my_ref_count) > 1)
        __TBB_Yield();
    __TBB_ASSERT(1 == my_ref_count, NULL);
}

void market::set_active_num_workers ( unsigned soft_limit ) {
    int old_requested, requested;
    market *m;
    // at least 1 worker is required to support starvation resistant tasks
    if( soft_limit == 0 )
        soft_limit = 1;

    {
        global_market_mutex_type::scoped_lock lock( theMarketMutex );
        if ( !theMarket )
            return; // actual value will be used at market creation
        m = theMarket;
        ++m->my_ref_count;
    }
    // have my_ref_count for market, use it safely
    {
        arenas_list_mutex_type::scoped_lock lock(m->my_arenas_list_mutex);
        __TBB_ASSERT(soft_limit <= m->my_num_workers_hard_limit, NULL);
        m->my_num_workers_soft_limit = soft_limit;
        requested = min(m->my_total_demand, (int)soft_limit);
        old_requested = m->my_num_workers_requested;
        m->my_num_workers_requested = requested;
        m->my_priority_levels[m->my_global_top_priority].workers_available = soft_limit;
        m->update_allotment(
#if __TBB_TASK_PRIORITY
            m->my_global_top_priority
#endif
            );
    }
    // Must be called outside of any locks
    if ( requested != old_requested )
        m->my_server->adjust_job_count_estimate( requested - old_requested );
    // release internal market reference to match ++m->my_ref_count above
    m->release();
}

bool governor::does_client_join_workers (const tbb::internal::rml::tbb_client &client) {
    return ((const market&)client).must_join_workers();
}

arena* market::create_arena ( int num_slots, int num_reserved_slots, size_t stack_size, bool default_concurrency_requested ) {
    __TBB_ASSERT( num_slots > 0, NULL );
    __TBB_ASSERT( num_reserved_slots <= num_slots, NULL );
    // Add public market reference for master thread/task_arena (that adds an internal reference in exchange).
    market &m = global_market( /*is_public=*/true, num_slots-num_reserved_slots, stack_size,
                               default_concurrency_requested ); 

    arena& a = arena::allocate_arena( m, num_slots, num_reserved_slots );
    // Add newly created arena into the existing market's list.
    arenas_list_mutex_type::scoped_lock lock(m.my_arenas_list_mutex);
    m.insert_arena_into_list(a);
    return &a;
}

/** This method must be invoked under my_arenas_list_mutex. **/
void market::detach_arena ( arena& a ) {
    __TBB_ASSERT( theMarket == this, "Global market instance was destroyed prematurely?" );
    __TBB_ASSERT( !a.my_slots[0].my_scheduler, NULL );
    remove_arena_from_list(a);
    if ( a.my_aba_epoch == my_arenas_aba_epoch )
        ++my_arenas_aba_epoch;
}

void market::try_destroy_arena ( arena* a, uintptr_t aba_epoch ) {
    // master thread holds reference to the market, so it cannot be destroyed at any moment here
    __TBB_ASSERT( this == theMarket, NULL );
    __TBB_ASSERT( a, NULL );
    arenas_list_mutex_type::scoped_lock lock(my_arenas_list_mutex);
    assert_market_valid();
#if __TBB_TASK_PRIORITY
    for ( int p = my_global_top_priority; p >= my_global_bottom_priority; --p ) {
        priority_level_info &pl = my_priority_levels[p];
        arena_list_type &my_arenas = pl.arenas;
#endif /* __TBB_TASK_PRIORITY */
        arena_list_type::iterator it = my_arenas.begin();
        for ( ; it != my_arenas.end(); ++it ) {
            if ( a == &*it ) {
                if ( it->my_aba_epoch == aba_epoch ) {
                    // Arena is alive
                    if ( !a->my_num_workers_requested && !a->my_references ) {
                        __TBB_ASSERT( !a->my_num_workers_allotted && (a->my_pool_state == arena::SNAPSHOT_EMPTY || !a->my_max_num_workers), "Inconsistent arena state" );
                        // Arena is abandoned. Destroy it.
                        detach_arena( *a );
                        lock.release();
                        a->free_arena();
                    }
                }
                return;
            }
        }
#if __TBB_TASK_PRIORITY
    }
#endif /* __TBB_TASK_PRIORITY */
}

/** This method must be invoked under my_arenas_list_mutex. **/
arena* market::arena_in_need ( arena_list_type &arenas, arena *&next ) {
    if ( arenas.empty() )
        return NULL;
    arena_list_type::iterator it = next;
    __TBB_ASSERT( it != arenas.end(), NULL );
    do {
        arena& a = *it;
        if ( ++it == arenas.end() )
            it = arenas.begin();
        if ( a.num_workers_active() < a.my_num_workers_allotted ) {
            a.my_references += arena::ref_worker;
            as_atomic(next) = &*it; // a subject for innocent data race under the reader lock
            // TODO: rework global round robin policy to local or random to avoid this write
            return &a;
        }
    } while ( it != next );
    return NULL;
}

void market::update_allotment ( arena_list_type& arenas, int workers_demand, int max_workers ) {
    __TBB_ASSERT( workers_demand, NULL );
    max_workers = min(workers_demand, max_workers);
    int carry = 0;
#if TBB_USE_ASSERT
    int assigned = 0;
#endif /* TBB_USE_ASSERT */
    arena_list_type::iterator it = arenas.begin();
    for ( ; it != arenas.end(); ++it ) {
        arena& a = *it;
        if ( a.my_num_workers_requested <= 0 ) {
            __TBB_ASSERT( !a.my_num_workers_allotted, NULL );
            continue;
        }
        int tmp = a.my_num_workers_requested * max_workers + carry;
        int allotted = tmp / workers_demand;
        carry = tmp % workers_demand;
        // a.my_num_workers_requested may temporarily exceed a.my_max_num_workers
        a.my_num_workers_allotted = min( allotted, (int)a.my_max_num_workers );
#if TBB_USE_ASSERT
        assigned += a.my_num_workers_allotted;
#endif /* TBB_USE_ASSERT */
    }
    __TBB_ASSERT( assigned <= workers_demand, NULL );
}

#if __TBB_TASK_PRIORITY
inline void market::update_global_top_priority ( intptr_t newPriority ) {
    GATHER_STATISTIC( ++governor::local_scheduler_if_initialized()->my_counters.market_prio_switches );
    my_global_top_priority = newPriority;
    my_priority_levels[newPriority].workers_available = my_num_workers_soft_limit;
    advance_global_reload_epoch();
}

inline void market::reset_global_priority () {
    my_global_bottom_priority = normalized_normal_priority;
    update_global_top_priority(normalized_normal_priority);
}

arena* market::arena_in_need ( arena* prev_arena )
{
    suppress_unused_warning(prev_arena);
    if( as_atomic(my_total_demand) <= 0 )
        return NULL;
    arenas_list_mutex_type::scoped_lock lock(my_arenas_list_mutex, /*is_writer=*/false);
    assert_market_valid();
    int p = my_global_top_priority;
    arena *a = NULL;
    do {
        priority_level_info &pl = my_priority_levels[p];
        a = arena_in_need( pl.arenas, pl.next_arena );
        // TODO: When refactoring task priority code, take into consideration the
        // __TBB_TRACK_PRIORITY_LEVEL_SATURATION sections from earlier versions of TBB
    } while ( !a && --p >= my_global_bottom_priority );
    return a;
}

void market::update_allotment ( intptr_t highest_affected_priority ) {
    intptr_t i = highest_affected_priority;
    int available = my_priority_levels[i].workers_available;
    for ( ; i >= my_global_bottom_priority; --i ) {
        priority_level_info &pl = my_priority_levels[i];
        pl.workers_available = available;
        if ( pl.workers_requested ) {
            update_allotment( pl.arenas, pl.workers_requested, available );
            available -= pl.workers_requested;
            if ( available < 0 ) {
                available = 0;
                break;
            }
        }
    }
    __TBB_ASSERT( i <= my_global_bottom_priority || !available, NULL );
    for ( --i; i >= my_global_bottom_priority; --i ) {
        priority_level_info &pl = my_priority_levels[i];
        pl.workers_available = 0;
        arena_list_type::iterator it = pl.arenas.begin();
        for ( ; it != pl.arenas.end(); ++it ) {
            __TBB_ASSERT( it->my_num_workers_requested >= 0 || !it->my_num_workers_allotted, NULL );
            it->my_num_workers_allotted = 0;
        }
    }
}
#endif /* __TBB_TASK_PRIORITY */

void market::adjust_demand ( arena& a, int delta ) {
    __TBB_ASSERT( theMarket, "market instance was destroyed prematurely?" );
    if ( !delta )
        return;
    my_arenas_list_mutex.lock();
    int prev_req = a.my_num_workers_requested;
    a.my_num_workers_requested += delta;
    if ( a.my_num_workers_requested <= 0 ) {
        a.my_num_workers_allotted = 0;
        if ( prev_req <= 0 ) {
            my_arenas_list_mutex.unlock();
            return;
        }
        delta = -prev_req;
    }
    else if ( prev_req < 0 ) {
        delta = a.my_num_workers_requested;
    }
    my_total_demand += delta;
#if !__TBB_TASK_PRIORITY
    update_allotment();
#else /* !__TBB_TASK_PRIORITY */
    intptr_t p = a.my_top_priority;
    priority_level_info &pl = my_priority_levels[p];
    pl.workers_requested += delta;
    __TBB_ASSERT( pl.workers_requested >= 0, NULL );
    if ( a.my_num_workers_requested <= 0 ) {
        if ( a.my_top_priority != normalized_normal_priority ) {
            GATHER_STATISTIC( ++governor::local_scheduler_if_initialized()->my_counters.arena_prio_resets );
            update_arena_top_priority( a, normalized_normal_priority );
        }
        a.my_bottom_priority = normalized_normal_priority;
    }
    if ( p == my_global_top_priority ) {
        if ( !pl.workers_requested ) {
            while ( --p >= my_global_bottom_priority && !my_priority_levels[p].workers_requested )
                continue;
            if ( p < my_global_bottom_priority )
                reset_global_priority();
            else
                update_global_top_priority(p);
        }
        update_allotment( my_global_top_priority );
    }
    else if ( p > my_global_top_priority ) {
        __TBB_ASSERT( pl.workers_requested > 0, NULL );
        // TODO: investigate if the following invariant is always valid
        __TBB_ASSERT( a.my_num_workers_requested >= 0, NULL );
        update_global_top_priority(p);
        a.my_num_workers_allotted = min( (int)my_num_workers_soft_limit, a.my_num_workers_requested );
        my_priority_levels[p - 1].workers_available = my_num_workers_soft_limit - a.my_num_workers_allotted;
        update_allotment( p - 1 );
    }
    else if ( p == my_global_bottom_priority ) {
        if ( !pl.workers_requested ) {
            while ( ++p <= my_global_top_priority && !my_priority_levels[p].workers_requested )
                continue;
            if ( p > my_global_top_priority )
                reset_global_priority();
            else
                my_global_bottom_priority = p;
        }
        else
            update_allotment( p );
    }
    else if ( p < my_global_bottom_priority ) {
        __TBB_ASSERT( a.my_num_workers_requested > 0, NULL );
        int prev_bottom = my_global_bottom_priority;
        my_global_bottom_priority = p;
        update_allotment( prev_bottom );
    }
    else {
        __TBB_ASSERT( my_global_bottom_priority < p && p < my_global_top_priority, NULL );
        update_allotment( p );
    }
    __TBB_ASSERT( my_global_top_priority >= a.my_top_priority || a.my_num_workers_requested<=0, NULL );
    assert_market_valid();
#endif /* !__TBB_TASK_PRIORITY */
    if ( delta > 0 ) {
        // can't overflow soft_limit, but remember values request by arenas in
        // my_total_demand to not prematurely release workers to RML
        if ( my_num_workers_requested+delta > (int)my_num_workers_soft_limit ) {
            delta = my_num_workers_soft_limit - my_num_workers_requested;
        }
    } else
        // the number of workers should not be decreased below my_total_demand
        if ( my_num_workers_requested+delta < my_total_demand )
            delta = min(my_total_demand, (int)my_num_workers_soft_limit) - my_num_workers_requested;
    my_num_workers_requested += delta;
    __TBB_ASSERT( my_num_workers_requested <= (int)my_num_workers_soft_limit, NULL );

    my_arenas_list_mutex.unlock();
    // Must be called outside of any locks
    my_server->adjust_job_count_estimate( delta );
    GATHER_STATISTIC( governor::local_scheduler_if_initialized() ? ++governor::local_scheduler_if_initialized()->my_counters.gate_switches : 0 );
}

void market::process( job& j ) {
    generic_scheduler& s = static_cast<generic_scheduler&>(j);
    arena *a = NULL;
    __TBB_ASSERT( governor::is_set(&s), NULL );
    enum {
        query_interval = 1000,
        first_interval = 1
    };
    for(int i = first_interval; ; i--) {
        while ( (a = arena_in_need(a)) )
        {
            a->process(s);
            i = first_interval;
        }
        // Workers leave market because there is no arena in need. It can happen earlier than
        // adjust_job_count_estimate() decreases my_slack and RML can put this thread to sleep.
        // It might result in a busy-loop checking for my_slack<0 and calling this method instantly.
        // first_interval>0 and the pause refines this spinning.
        if( i > 0 )
            prolonged_pause();
        else
#if !__TBB_SLEEP_PERMISSION
            break;
#else
        { // i == 0
#if __TBB_TASK_PRIORITY
            arena_list_type &al = my_priority_levels[my_global_top_priority].arenas;
#else /* __TBB_TASK_PRIORITY */
            arena_list_type &al = my_arenas;
#endif /* __TBB_TASK_PRIORITY */
            if( al.empty() ) // races if any are innocent TODO: replace by an RML query interface
                break; // no arenas left, perhaps going to shut down
            if( the_global_observer_list.ask_permission_to_leave() )
                break; // go sleep
            __TBB_Yield();
            i = query_interval;
        }
#endif// !__TBB_SLEEP_PERMISSION
    }
    GATHER_STATISTIC( ++s.my_counters.market_roundtrips );
}

void market::cleanup( job& j ) {
    __TBB_ASSERT( theMarket != this, NULL );
    generic_scheduler& s = static_cast<generic_scheduler&>(j);
    generic_scheduler* mine = governor::local_scheduler_if_initialized();
    __TBB_ASSERT( !mine || mine->is_worker(), NULL );
    if( mine!=&s ) {
        governor::assume_scheduler( &s );
        generic_scheduler::cleanup_worker( &s, mine!=NULL );
        governor::assume_scheduler( mine );
    } else {
        generic_scheduler::cleanup_worker( &s, true );
    }
}

void market::acknowledge_close_connection() {
    destroy();
}

::rml::job* market::create_one_job() {
    unsigned index = ++my_first_unused_worker_idx;
    __TBB_ASSERT( index > 0, NULL );
    ITT_THREAD_SET_NAME(_T("TBB Worker Thread"));
    // index serves as a hint decreasing conflicts between workers when they migrate between arenas
    generic_scheduler* s = generic_scheduler::create_worker( *this, index );
#if __TBB_TASK_GROUP_CONTEXT
    __TBB_ASSERT( index <= my_num_workers_hard_limit, NULL );
    __TBB_ASSERT( !my_workers[index - 1], NULL );
    my_workers[index - 1] = s;
#endif /* __TBB_TASK_GROUP_CONTEXT */
    return s;
}

#if __TBB_TASK_PRIORITY
void market::update_arena_top_priority ( arena& a, intptr_t new_priority ) {
    GATHER_STATISTIC( ++governor::local_scheduler_if_initialized()->my_counters.arena_prio_switches );
    __TBB_ASSERT( a.my_top_priority != new_priority, NULL );
    priority_level_info &prev_level = my_priority_levels[a.my_top_priority],
                        &new_level = my_priority_levels[new_priority];
    remove_arena_from_list(a);
    a.my_top_priority = new_priority;
    insert_arena_into_list(a);
    ++a.my_reload_epoch; // TODO: synch with global reload epoch in order to optimize usage of local reload epoch
    prev_level.workers_requested -= a.my_num_workers_requested;
    new_level.workers_requested += a.my_num_workers_requested;
    __TBB_ASSERT( prev_level.workers_requested >= 0 && new_level.workers_requested >= 0, NULL );
}

bool market::lower_arena_priority ( arena& a, intptr_t new_priority, uintptr_t old_reload_epoch ) {
    // TODO: replace the lock with a try_lock loop which performs a double check of the epoch
    arenas_list_mutex_type::scoped_lock lock(my_arenas_list_mutex);
    if ( a.my_reload_epoch != old_reload_epoch ) {
        assert_market_valid();
        return false;
    }
    __TBB_ASSERT( a.my_top_priority > new_priority, NULL );
    __TBB_ASSERT( my_global_top_priority >= a.my_top_priority, NULL );

    intptr_t p = a.my_top_priority;
    update_arena_top_priority( a, new_priority );
    if ( a.my_num_workers_requested > 0 ) {
        if ( my_global_bottom_priority > new_priority ) {
            my_global_bottom_priority = new_priority;
        }
        if ( p == my_global_top_priority && !my_priority_levels[p].workers_requested ) {
            // Global top level became empty
            for ( --p; !my_priority_levels[p].workers_requested; --p ) continue;
            __TBB_ASSERT( p >= my_global_bottom_priority, NULL );
            update_global_top_priority(p);
        }
        update_allotment( p );
    }

    __TBB_ASSERT( my_global_top_priority >= a.my_top_priority, NULL );
    assert_market_valid();
    return true;
}

bool market::update_arena_priority ( arena& a, intptr_t new_priority ) {
    // TODO: do not acquire this global lock while checking arena's state.
    arenas_list_mutex_type::scoped_lock lock(my_arenas_list_mutex);

    __TBB_ASSERT( my_global_top_priority >= a.my_top_priority || a.my_num_workers_requested <= 0, NULL );
    assert_market_valid();
    if ( a.my_top_priority == new_priority ) {
        return false;
    }
    else if ( a.my_top_priority > new_priority ) {
        if ( a.my_bottom_priority > new_priority )
            a.my_bottom_priority = new_priority;
        return false;
    }
    else if ( a.my_num_workers_requested <= 0 ) {
        return false;
    }

    __TBB_ASSERT( my_global_top_priority >= a.my_top_priority, NULL );

    intptr_t p = a.my_top_priority;
    intptr_t highest_affected_level = max(p, new_priority);
    update_arena_top_priority( a, new_priority );

    if ( my_global_top_priority < new_priority ) {
        update_global_top_priority(new_priority);
    }
    else if ( my_global_top_priority == new_priority ) {
        advance_global_reload_epoch();
    }
    else {
        __TBB_ASSERT( new_priority < my_global_top_priority, NULL );
        __TBB_ASSERT( new_priority > my_global_bottom_priority, NULL );
        if ( p == my_global_top_priority && !my_priority_levels[p].workers_requested ) {
            // Global top level became empty
            __TBB_ASSERT( my_global_bottom_priority < p, NULL );
            for ( --p; !my_priority_levels[p].workers_requested; --p ) continue;
            __TBB_ASSERT( p >= new_priority, NULL );
            update_global_top_priority(p);
            highest_affected_level = p;
        }
    }
    if ( p == my_global_bottom_priority ) {
        // Arena priority was increased from the global bottom level.
        __TBB_ASSERT( p < new_priority, NULL );                     // n
        __TBB_ASSERT( new_priority <= my_global_top_priority, NULL );
        while ( !my_priority_levels[my_global_bottom_priority].workers_requested )
            ++my_global_bottom_priority;
        __TBB_ASSERT( my_global_bottom_priority <= new_priority, NULL );
        __TBB_ASSERT( my_priority_levels[my_global_bottom_priority].workers_requested > 0, NULL );
    }
    update_allotment( highest_affected_level );

    __TBB_ASSERT( my_global_top_priority >= a.my_top_priority, NULL );
    assert_market_valid();
    return true;
}
#endif /* __TBB_TASK_PRIORITY */

} // namespace internal
} // namespace tbb
