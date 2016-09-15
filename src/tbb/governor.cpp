/*
    Copyright 2005-2016 Intel Corporation.  All Rights Reserved.

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

#include <stdio.h>
#include <stdlib.h>
#include "governor.h"
#include "tbb_main.h"
#include "scheduler.h"
#include "market.h"
#include "arena.h"

#include "tbb/task_scheduler_init.h"

#include "dynamic_link.h"

namespace tbb {
namespace internal {

//------------------------------------------------------------------------
// governor
//------------------------------------------------------------------------

#if __TBB_SURVIVE_THREAD_SWITCH
// Support for interoperability with Intel(R) Cilk(TM) Plus.

#if _WIN32
#define CILKLIB_NAME "cilkrts20.dll"
#else
#define CILKLIB_NAME "libcilkrts.so"
#endif

//! Handler for interoperation with cilkrts library.
static __cilk_tbb_retcode (*watch_stack_handler)(struct __cilk_tbb_unwatch_thunk* u,
                                                 struct __cilk_tbb_stack_op_thunk o);

//! Table describing how to link the handlers.
static const dynamic_link_descriptor CilkLinkTable[] = {
    { "__cilkrts_watch_stack", (pointer_to_handler*)(void*)(&watch_stack_handler) }
};

static atomic<do_once_state> cilkrts_load_state;

bool initialize_cilk_interop() {
    // Pinning can fail. This is a normal situation, and means that the current
    // thread does not use cilkrts and consequently does not need interop.
    return dynamic_link( CILKLIB_NAME, CilkLinkTable, 1,  /*handle=*/0, DYNAMIC_LINK_GLOBAL );
}
#endif /* __TBB_SURVIVE_THREAD_SWITCH */

namespace rml {
    tbb_server* make_private_server( tbb_client& client );
}

void governor::acquire_resources () {
#if USE_PTHREAD
    int status = theTLS.create(auto_terminate);
#else
    int status = theTLS.create();
#endif
    if( status )
        handle_perror(status, "TBB failed to initialize task scheduler TLS\n");
    is_speculation_enabled = cpu_has_speculation();
}

void governor::release_resources () {
    theRMLServerFactory.close();
    destroy_process_mask();
#if TBB_USE_ASSERT
    if( __TBB_InitOnce::initialization_done() && theTLS.get() ) 
        runtime_warning( "TBB is unloaded while tbb::task_scheduler_init object is alive?" );
#endif
    int status = theTLS.destroy();
    if( status )
        runtime_warning("failed to destroy task scheduler TLS: %s", strerror(status));
    dynamic_unlink_all();
}

rml::tbb_server* governor::create_rml_server ( rml::tbb_client& client ) {
    rml::tbb_server* server = NULL;
    if( !UsePrivateRML ) {
        ::rml::factory::status_type status = theRMLServerFactory.make_server( server, client );
        if( status != ::rml::factory::st_success ) {
            UsePrivateRML = true;
            runtime_warning( "rml::tbb_factory::make_server failed with status %x, falling back on private rml", status );
        }
    }
    if ( !server ) {
        __TBB_ASSERT( UsePrivateRML, NULL );
        server = rml::make_private_server( client );
    }
    __TBB_ASSERT( server, "Failed to create RML server" );
    return server;
}


uintptr_t governor::tls_value_of( generic_scheduler* s ) {
    __TBB_ASSERT( (uintptr_t(s)&1) == 0, "Bad pointer to the scheduler" );
    // LSB marks the scheduler initialized with arena
    return uintptr_t(s) | uintptr_t((s && (s->my_arena || s->is_worker()))? 1 : 0);
}

void governor::assume_scheduler( generic_scheduler* s ) {
    theTLS.set( tls_value_of(s) );
}

bool governor::is_set( generic_scheduler* s ) {
    return theTLS.get() == tls_value_of(s);
}

void governor::sign_on(generic_scheduler* s) {
    __TBB_ASSERT( is_set(NULL) && s, NULL );
    assume_scheduler( s );
#if __TBB_SURVIVE_THREAD_SWITCH
    if( watch_stack_handler ) {
        __cilk_tbb_stack_op_thunk o;
        o.routine = &stack_op_handler;
        o.data = s;
        if( (*watch_stack_handler)(&s->my_cilk_unwatch_thunk, o) ) {
            // Failed to register with cilkrts, make sure we are clean
            s->my_cilk_unwatch_thunk.routine = NULL;
        }
#if TBB_USE_ASSERT
        else
            s->my_cilk_state = generic_scheduler::cs_running;
#endif /* TBB_USE_ASSERT */
    }
#endif /* __TBB_SURVIVE_THREAD_SWITCH */
    __TBB_ASSERT( is_set(s), NULL );
}

void governor::sign_off(generic_scheduler* s) {
    suppress_unused_warning(s);
    __TBB_ASSERT( is_set(s), "attempt to unregister a wrong scheduler instance" );
    assume_scheduler(NULL);
#if __TBB_SURVIVE_THREAD_SWITCH
    __cilk_tbb_unwatch_thunk &ut = s->my_cilk_unwatch_thunk;
    if ( ut.routine )
       (*ut.routine)(ut.data);
#endif /* __TBB_SURVIVE_THREAD_SWITCH */
}

void governor::setBlockingTerminate(const task_scheduler_init *tsi) {
    __TBB_ASSERT(!IsBlockingTerminationInProgress, "It's impossible to create task_scheduler_init while blocking termination is in progress.");
    if (BlockingTSI)
        throw_exception(eid_blocking_sch_init);
    BlockingTSI = tsi;
}

void governor::one_time_init() {
    if( !__TBB_InitOnce::initialization_done() )
        DoOneTimeInitializations();
#if __TBB_SURVIVE_THREAD_SWITCH
    atomic_do_once( &initialize_cilk_interop, cilkrts_load_state );
#endif /* __TBB_SURVIVE_THREAD_SWITCH */
}

generic_scheduler* governor::init_scheduler_weak() {
    one_time_init();
    __TBB_ASSERT( is_set(NULL), "TLS contains a scheduler?" );
    generic_scheduler* s = generic_scheduler::create_master( NULL ); // without arena
    s->my_auto_initialized = true;
    return s;
}

generic_scheduler* governor::init_scheduler( int num_threads, stack_size_type stack_size, bool auto_init ) {
    one_time_init();
    if ( uintptr_t v = theTLS.get() ) {
        generic_scheduler* s = tls_scheduler_of( v );
        if ( (v&1) == 0 ) { // TLS holds scheduler instance without arena
            __TBB_ASSERT( s->my_ref_count == 1, "weakly initialized scheduler must have refcount equal to 1" );
            __TBB_ASSERT( !s->my_arena, "weakly initialized scheduler  must have no arena" );
            __TBB_ASSERT( s->my_auto_initialized, "weakly initialized scheduler is supposed to be auto-initialized" );
            s->attach_arena( market::create_arena( default_num_threads(), 1, 0 ), 0, /*is_master*/true );
            __TBB_ASSERT( s->my_arena_index == 0, "Master thread must occupy the first slot in its arena" );
            s->my_arena_slot->my_scheduler = s;
            s->my_arena->my_default_ctx = s->default_context(); // it also transfers implied ownership
            // Mark the scheduler as fully initialized
            assume_scheduler( s );
        }
        // Increment refcount only for explicit instances of task_scheduler_init.
        if ( !auto_init ) s->my_ref_count += 1;
        __TBB_ASSERT( s->my_arena, "scheduler is not initialized fully" );
        return s;
    }
    // Create new scheduler instance with arena
    if( num_threads == task_scheduler_init::automatic )
        num_threads = default_num_threads();
    arena *a = market::create_arena( num_threads, 1, stack_size );
    generic_scheduler* s = generic_scheduler::create_master( a );
    __TBB_ASSERT(s, "Somehow a local scheduler creation for a master thread failed");
    __TBB_ASSERT( is_set(s), NULL );
    s->my_auto_initialized = auto_init;
    return s;
}

void governor::terminate_scheduler( generic_scheduler* s, const task_scheduler_init* tsi_ptr ) {
    __TBB_ASSERT( is_set(s), "Attempt to terminate non-local scheduler instance" );
    if (--(s->my_ref_count)) {
        // can't throw exception, because this is on dtor's call chain
        __TBB_ASSERT_RELEASE( BlockingTSI!=tsi_ptr,
                              "Attempt to terminate nested scheduler in blocking mode" );
    } else {
        bool needs_wait_workers = false;
        if ( BlockingTSI==tsi_ptr ) {
            needs_wait_workers = true;
            BlockingTSI = NULL;
#if TBB_USE_ASSERT
            IsBlockingTerminationInProgress = true;
#endif
        }
        s->cleanup_master( needs_wait_workers );
        __TBB_ASSERT( is_set(NULL), "cleanup_master has not cleared its TLS slot" );
#if TBB_USE_ASSERT
        if ( needs_wait_workers ) {
            __TBB_ASSERT( IsBlockingTerminationInProgress, NULL );
            IsBlockingTerminationInProgress = false;
        }
#endif
    }
}

void governor::auto_terminate(void* arg){
    generic_scheduler* s = tls_scheduler_of( uintptr_t(arg) ); // arg is equivalent to theTLS.get()
    if( s && s->my_auto_initialized ) {
        if( !--(s->my_ref_count) ) {
            // If the TLS slot is already cleared by OS or underlying concurrency
            // runtime, restore its value.
            if( !is_set(s) )
                assume_scheduler(s);
            s->cleanup_master( /*needs_wait_workers=*/false );
            __TBB_ASSERT( is_set(NULL), "cleanup_master has not cleared its TLS slot" );
        }
    }
}

void governor::print_version_info () {
    if ( UsePrivateRML )
        PrintExtraVersionInfo( "RML", "private" );
    else {
        PrintExtraVersionInfo( "RML", "shared" );
        theRMLServerFactory.call_with_server_info( PrintRMLVersionInfo, (void*)"" );
    }
#if __TBB_SURVIVE_THREAD_SWITCH
    if( watch_stack_handler )
        PrintExtraVersionInfo( "CILK", CILKLIB_NAME );
#endif /* __TBB_SURVIVE_THREAD_SWITCH */
}

void governor::initialize_rml_factory () {
    ::rml::factory::status_type res = theRMLServerFactory.open();
    UsePrivateRML = res != ::rml::factory::st_success;
}

#if __TBB_SURVIVE_THREAD_SWITCH
__cilk_tbb_retcode governor::stack_op_handler( __cilk_tbb_stack_op op, void* data ) {
    __TBB_ASSERT(data,NULL);
    generic_scheduler* s = static_cast<generic_scheduler*>(data);
#if TBB_USE_ASSERT
    void* current = local_scheduler_if_initialized();
#if _WIN32||_WIN64
    uintptr_t thread_id = GetCurrentThreadId();
#else
    uintptr_t thread_id = uintptr_t(pthread_self());
#endif

#endif /* TBB_USE_ASSERT */
    switch( op ) {
        default:
            __TBB_ASSERT( 0, "invalid op" );
        case CILK_TBB_STACK_ADOPT: {
            __TBB_ASSERT( !current && s->my_cilk_state==generic_scheduler::cs_limbo ||
                          current==s && s->my_cilk_state==generic_scheduler::cs_running, "invalid adoption" );
#if TBB_USE_ASSERT
            if( current==s )
                runtime_warning( "redundant adoption of %p by thread %p\n", s, (void*)thread_id );
            s->my_cilk_state = generic_scheduler::cs_running;
#endif /* TBB_USE_ASSERT */
            assume_scheduler( s );
            break;
        }
        case CILK_TBB_STACK_ORPHAN: {
            __TBB_ASSERT( current==s && s->my_cilk_state==generic_scheduler::cs_running, "invalid orphaning" );
#if TBB_USE_ASSERT
            s->my_cilk_state = generic_scheduler::cs_limbo;
#endif /* TBB_USE_ASSERT */
            assume_scheduler(NULL);
            break;
        }
        case CILK_TBB_STACK_RELEASE: {
            __TBB_ASSERT( !current && s->my_cilk_state==generic_scheduler::cs_limbo ||
                          current==s && s->my_cilk_state==generic_scheduler::cs_running, "invalid release" );
#if TBB_USE_ASSERT
            s->my_cilk_state = generic_scheduler::cs_freed;
#endif /* TBB_USE_ASSERT */
            s->my_cilk_unwatch_thunk.routine = NULL;
            auto_terminate( s );
        }
    }
    return 0;
}
#endif /* __TBB_SURVIVE_THREAD_SWITCH */

} // namespace internal

//------------------------------------------------------------------------
// task_scheduler_init
//------------------------------------------------------------------------

using namespace internal;

/** Left out-of-line for the sake of the backward binary compatibility **/
void task_scheduler_init::initialize( int number_of_threads ) {
    initialize( number_of_threads, 0 );
}

void task_scheduler_init::initialize( int number_of_threads, stack_size_type thread_stack_size ) {
#if __TBB_TASK_GROUP_CONTEXT && TBB_USE_EXCEPTIONS
    uintptr_t new_mode = thread_stack_size & propagation_mode_mask;
#endif
    thread_stack_size &= ~(stack_size_type)propagation_mode_mask;
    if( number_of_threads!=deferred ) {
        bool blocking_terminate = false;
        if (my_scheduler == (scheduler*)wait_workers_in_terminate_flag) {
            blocking_terminate = true;
            my_scheduler = NULL;
        }
        __TBB_ASSERT_RELEASE( !my_scheduler, "task_scheduler_init already initialized" );
        __TBB_ASSERT_RELEASE( number_of_threads==automatic || number_of_threads > 0,
                    "number_of_threads for task_scheduler_init must be automatic or positive" );
        if (blocking_terminate)
            governor::setBlockingTerminate(this);
        internal::generic_scheduler *s = governor::init_scheduler( number_of_threads, thread_stack_size, /*auto_init=*/false );
#if __TBB_TASK_GROUP_CONTEXT && TBB_USE_EXCEPTIONS
        if ( s->master_outermost_level() ) {
            uintptr_t &vt = s->default_context()->my_version_and_traits;
            uintptr_t prev_mode = vt & task_group_context::exact_exception ? propagation_mode_exact : 0;
            vt = new_mode & propagation_mode_exact ? vt | task_group_context::exact_exception
                    : new_mode & propagation_mode_captured ? vt & ~task_group_context::exact_exception : vt;
            // Use least significant bit of the scheduler pointer to store previous mode.
            // This is necessary when components compiled with different compilers and/or
            // TBB versions initialize the
            my_scheduler = static_cast<scheduler*>((generic_scheduler*)((uintptr_t)s | prev_mode));
        }
        else
#endif /* __TBB_TASK_GROUP_CONTEXT && TBB_USE_EXCEPTIONS */
            my_scheduler = s;
    } else {
        __TBB_ASSERT_RELEASE( !thread_stack_size, "deferred initialization ignores stack size setting" );
    }
}

void task_scheduler_init::terminate() {
#if __TBB_TASK_GROUP_CONTEXT && TBB_USE_EXCEPTIONS
    uintptr_t prev_mode = (uintptr_t)my_scheduler & propagation_mode_exact;
    my_scheduler = (scheduler*)((uintptr_t)my_scheduler & ~(uintptr_t)propagation_mode_exact);
#endif /* __TBB_TASK_GROUP_CONTEXT && TBB_USE_EXCEPTIONS */
    generic_scheduler* s = static_cast<generic_scheduler*>(my_scheduler);
    my_scheduler = NULL;
    __TBB_ASSERT_RELEASE( s, "task_scheduler_init::terminate without corresponding task_scheduler_init::initialize()");
#if __TBB_TASK_GROUP_CONTEXT && TBB_USE_EXCEPTIONS
    if ( s->master_outermost_level() ) {
        uintptr_t &vt = s->default_context()->my_version_and_traits;
        vt = prev_mode & propagation_mode_exact ? vt | task_group_context::exact_exception
                                        : vt & ~task_group_context::exact_exception;
    }
#endif /* __TBB_TASK_GROUP_CONTEXT && TBB_USE_EXCEPTIONS */
    governor::terminate_scheduler(s, this);
}

int task_scheduler_init::default_num_threads() {
    return governor::default_num_threads();
}

} // namespace tbb
