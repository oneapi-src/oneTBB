/*
    Copyright (c) 2005-2020 Intel Corporation

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

#include "governor.h"
#include "main.h"
#include "thread_data.h"
#include "market.h"
#include "arena.h"
#include "dynamic_link.h"

#include "tbb/task_group.h"
#include "task_dispatcher.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <atomic>

namespace tbb {
namespace detail {
namespace r1 {

namespace rml {
tbb_server* make_private_server( tbb_client& client );
} // namespace rml

//------------------------------------------------------------------------
// governor
//------------------------------------------------------------------------

void governor::acquire_resources () {
#if __TBB_USE_POSIX
    int status = theTLS.create(auto_terminate);
#else
    int status = theTLS.create();
#endif
    if( status )
        handle_perror(status, "TBB failed to initialize task scheduler TLS\n");
    is_speculation_enabled = cpu_has_speculation();
    is_rethrow_broken = gcc_rethrow_exception_broken();
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
        runtime_warning("failed to destroy task scheduler TLS: %s", std::strerror(status));
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

void governor::one_time_init() {
    if ( !__TBB_InitOnce::initialization_done() ) {
        DoOneTimeInitialization();
    }
}

/*
    There is no portable way to get stack base address in Posix, however the modern
    Linux versions provide pthread_attr_np API that can be used  to obtain thread's
    stack size and base address. Unfortunately even this function does not provide
    enough information for the main thread on IA-64 architecture (RSE spill area
    and memory stack are allocated as two separate discontinuous chunks of memory),
    and there is no portable way to discern the main and the secondary threads.
    Thus for macOS* and IA-64 architecture for Linux* OS we use the TBB worker stack size for
    all threads and use the current stack top as the stack base. This simplified
    approach is based on the following assumptions:
    1) If the default stack size is insufficient for the user app needs, the
    required amount will be explicitly specified by the user at the point of the
    TBB scheduler initialization (as an argument to tbb::task_scheduler_init
    constructor).
    2) When a master thread initializes the scheduler, it has enough space on its
    stack. Here "enough" means "at least as much as worker threads have".
    3) If the user app strives to conserve the memory by cutting stack size, it
    should do this for TBB workers too (as in the #1).
*/
static std::uintptr_t get_stack_base(std::size_t stack_size) {
    // Stacks are growing top-down. Highest address is called "stack base",
    // and the lowest is "stack limit".
#if USE_WINTHREAD
    suppress_unused_warning(stack_size);
    NT_TIB* pteb = (NT_TIB*)NtCurrentTeb();
    __TBB_ASSERT(&pteb < pteb->StackBase && &pteb > pteb->StackLimit, "invalid stack info in TEB");
    return pteb->StackBase;
#else /* USE_PTHREAD */
    // There is no portable way to get stack base address in Posix, so we use
    // non-portable method (on all modern Linux) or the simplified approach
    // based on the common sense assumptions. The most important assumption
    // is that the main thread's stack size is not less than that of other threads.

    // Points to the lowest addressable byte of a stack.
    void* stack_limit = nullptr;
#if __linux__ && !__bg__
    size_t np_stack_size = 0;
    pthread_attr_t np_attr_stack;
    if (0 == pthread_getattr_np(pthread_self(), &np_attr_stack)) {
        if (0 == pthread_attr_getstack(&np_attr_stack, &stack_limit, &np_stack_size)) {
            __TBB_ASSERT( &stack_limit > stack_limit, "stack size must be positive" );
        }
        pthread_attr_destroy(&np_attr_stack);
    }
#endif /* __linux__ */
    std::uintptr_t stack_base{};
    if (stack_limit) {
        stack_base = reinterpret_cast<std::uintptr_t>(stack_limit) + stack_size;
    } else {
        // Use an anchor as a base stack address.
        int anchor{};
        stack_base = reinterpret_cast<std::uintptr_t>(&anchor);
    }
    return stack_base;
#endif /* USE_PTHREAD */
}

void governor::init_external_thread() {
    one_time_init();
    // Create new scheduler instance with arena
    int num_slots = default_num_threads();
    // TODO_REVAMP: support arena-less masters
    int num_reserved_slots = 1;
    unsigned arena_priority_level = 1; // corresponds to tbb::task_arena::priority::normal
    std::size_t stack_size = 0;
    arena& a = *market::create_arena(num_slots, num_reserved_slots, arena_priority_level, stack_size);
    // We need an internal reference to the market. TODO: is it legacy?
    market::global_market(false);
    // Master thread always occupies the first slot
    thread_data& td = *new(cache_aligned_allocate(sizeof(thread_data))) thread_data(0, false);
    td.attach_arena(a, /*slot index*/ 0);

    stack_size = a.my_market->worker_stack_size();
    std::uintptr_t stack_base = get_stack_base(stack_size);
    task_dispatcher& task_disp = td.my_arena_slot->default_task_dispatcher();
    task_disp.set_stealing_threshold(calculate_stealing_threshold(stack_base, stack_size));
    td.attach_task_dispatcher(task_disp);

    td.my_arena_slot->occupy();
    a.my_market->add_external_thread(td);
    set_thread_data(td);
}

void governor::auto_terminate(void* tls) {
    __TBB_ASSERT(get_thread_data_if_initialized() == nullptr ||
        get_thread_data_if_initialized() == tls, NULL);
    if (tls) {
        thread_data* td = static_cast<thread_data*>(tls);

        // Only external thread can be inside an arena during termination.
        if (td->my_arena_slot) {
            arena* a = td->my_arena;
            market* m = a->my_market;

            a->my_observers.notify_exit_observers(td->my_last_observer, td->my_is_worker);

            td->my_task_dispatcher->m_stealing_threshold = 0;
            td->detach_task_dispatcher();
            td->my_arena_slot->release();
            // Release an arena
            a->on_thread_leaving<arena::ref_external>();

            m->remove_external_thread(*td);
            // If there was an associated arena, it added a public market reference
            m->release( /*is_public*/ true, /*blocking_terminate*/ false);
        }

        td->~thread_data();
        cache_aligned_deallocate(td);

        clear_thread_data();
    }
    __TBB_ASSERT(get_thread_data_if_initialized() == nullptr, NULL);
}

void governor::initialize_rml_factory () {
    ::rml::factory::status_type res = theRMLServerFactory.open();
    UsePrivateRML = res != ::rml::factory::st_success;
}

#if __TBB_NUMA_SUPPORT

#if __TBB_WEAK_SYMBOLS_PRESENT
#pragma weak __TBB_internal_initialize_numa_topology
#pragma weak __TBB_internal_allocate_binding_handler
#pragma weak __TBB_internal_deallocate_binding_handler
#pragma weak __TBB_internal_bind_to_node
#pragma weak __TBB_internal_restore_affinity

extern "C" {
void __TBB_internal_initialize_numa_topology(
    size_t groups_num, int& nodes_count, int*& indexes_list, int*& concurrency_list );

//TODO: consider renaming to `create_binding_handler` and `destroy_binding_handler`
binding_handler* __TBB_internal_allocate_binding_handler( int slot_num );
void __TBB_internal_deallocate_binding_handler( binding_handler* handler_ptr );

void __TBB_internal_bind_to_node( binding_handler* handler_ptr, int slot_num, int numa_id );
void __TBB_internal_restore_affinity( binding_handler* handler_ptr, int slot_num );
}
#endif /* __TBB_WEAK_SYMBOLS_PRESENT */

// Handlers for communication with TBBbind
#if _WIN32 || _WIN64 || __linux__
static void (*initialize_numa_topology_ptr)(
    size_t groups_num, int& nodes_count, int*& indexes_list, int*& concurrency_list ) = NULL;
#endif /* _WIN32 || _WIN64 || __linux__ */

static binding_handler* (*allocate_binding_handler_ptr)( int slot_num ) = NULL;
static void (*deallocate_binding_handler_ptr)( binding_handler* handler_ptr ) = NULL;

static void (*bind_to_node_ptr)( binding_handler* handler_ptr, int slot_num, int numa_id ) = NULL;
static void (*restore_affinity_ptr)( binding_handler* handler_ptr, int slot_num ) = NULL;

#if _WIN32 || _WIN64 || __linux__
// Table describing how to link the handlers.
static const dynamic_link_descriptor TbbBindLinkTable[] = {
    DLD(__TBB_internal_initialize_numa_topology, initialize_numa_topology_ptr),
    DLD(__TBB_internal_allocate_binding_handler, allocate_binding_handler_ptr),
    DLD(__TBB_internal_deallocate_binding_handler, deallocate_binding_handler_ptr),
    DLD(__TBB_internal_bind_to_node, bind_to_node_ptr),
    DLD(__TBB_internal_restore_affinity, restore_affinity_ptr)
};

static const unsigned LinkTableSize = 5;

#if TBB_USE_DEBUG
#define DEBUG_SUFFIX "_debug"
#else
#define DEBUG_SUFFIX
#endif /* TBB_USE_DEBUG */

#if _WIN32 || _WIN64
#define TBBBIND_NAME "tbbbind" DEBUG_SUFFIX ".dll"
#elif __linux__
#define TBBBIND_NAME "libtbbbind" DEBUG_SUFFIX __TBB_STRING(.so.3)
#endif /* __linux__ */
#endif /* _WIN32 || _WIN64 || __linux__ */

// Stubs that will be used if TBBbind library is unavailable.
static binding_handler* dummy_allocate_binding_handler ( int ) { return NULL; }
static void dummy_deallocate_binding_handler ( binding_handler* ) { }
static void dummy_bind_to_node ( binding_handler*, int, int ) { }
static void dummy_restore_affinity ( binding_handler*, int ) { }

// Representation of NUMA topology information on the TBB side.
// NUMA topology may be initialized by third-party component (e.g. hwloc)
// or just filled by default stubs (1 NUMA node with 0 index and
// default_num_threads value as default_concurrency).
namespace numa_topology {
namespace {
int  numa_nodes_count = 0;
int* numa_indexes = NULL;
int* default_concurrency_list = NULL;
static std::atomic<do_once_state> numa_topology_init_state;
} // internal namespace

// Tries to load TBBbind library API, if success, gets NUMA topology information from it,
// in another case, fills NUMA topology by stubs.
// TODO: Add TBBbind loading status if TBB_VERSION is set.
void initialization_impl() {
    governor::one_time_init();

#if _WIN32 || _WIN64 || __linux__
    bool load_tbbbind = true;
#if _WIN32 && !_WIN64
    // For 32-bit Windows applications, process affinity masks can only support up to 32 logical CPUs.
    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);
    load_tbbbind = si.dwNumberOfProcessors <= 32;
#endif /* _WIN32 && !_WIN64 */

    if (load_tbbbind && dynamic_link(TBBBIND_NAME, TbbBindLinkTable, LinkTableSize)) {
        int number_of_groups = 1;
#if _WIN32 || _WIN64
        number_of_groups = NumberOfProcessorGroups();
#endif /* _WIN32 || _WIN64 */
        initialize_numa_topology_ptr(
            number_of_groups, numa_nodes_count, numa_indexes, default_concurrency_list);

        if (numa_nodes_count==1 && numa_indexes[0] >= 0) {
            __TBB_ASSERT(default_concurrency_list[numa_indexes[0]] == (int)governor::default_num_threads(),
                "default_concurrency() should be equal to governor::default_num_threads() on single"
                "NUMA node systems.");
        }
        return;
    }
#endif /* _WIN32 || _WIN64 || __linux__ */

    static int dummy_index = -1;
    static int dummy_concurrency = governor::default_num_threads();

    numa_nodes_count = 1;
    numa_indexes = &dummy_index;
    default_concurrency_list = &dummy_concurrency;

    allocate_binding_handler_ptr = dummy_allocate_binding_handler;
    deallocate_binding_handler_ptr = dummy_deallocate_binding_handler;

    bind_to_node_ptr = dummy_bind_to_node;
    restore_affinity_ptr = dummy_restore_affinity;
}

void initialize() {
    atomic_do_once(initialization_impl, numa_topology_init_state);
}
} // namespace numa_topology

binding_handler* construct_binding_handler(int slot_num) {
    __TBB_ASSERT(allocate_binding_handler_ptr, "tbbbind loading was not performed");
    return allocate_binding_handler_ptr(slot_num);
}

void destroy_binding_handler(binding_handler* handler_ptr) {
    __TBB_ASSERT(deallocate_binding_handler_ptr, "tbbbind loading was not performed");
    deallocate_binding_handler_ptr(handler_ptr);
}

void bind_thread_to_node(binding_handler* handler_ptr, int slot_num , int numa_id) {
    __TBB_ASSERT(slot_num >= 0, "Negative thread index");
    __TBB_ASSERT(bind_to_node_ptr, "tbbbind loading was not performed");
    bind_to_node_ptr(handler_ptr, slot_num, numa_id);
}

void restore_affinity_mask(binding_handler* handler_ptr, int slot_num) {
    __TBB_ASSERT(slot_num >= 0, "Negative thread index");
    __TBB_ASSERT(restore_affinity_ptr, "tbbbind loading was not performed");
    restore_affinity_ptr(handler_ptr, slot_num);
}

unsigned __TBB_EXPORTED_FUNC numa_node_count() {
    numa_topology::initialize();
    return numa_topology::numa_nodes_count;
}
void __TBB_EXPORTED_FUNC fill_numa_indices(int* index_array) {
    numa_topology::initialize();
    for (int i = 0; i < numa_topology::numa_nodes_count; i++) {
        index_array[i] = numa_topology::numa_indexes[i];
    }
}
int __TBB_EXPORTED_FUNC numa_default_concurrency(int node_id) {
    if (node_id >= 0) {
        numa_topology::initialize();
        return numa_topology::default_concurrency_list[node_id];
    }
    return governor::default_num_threads();
}
#endif /* __TBB_NUMA_SUPPORT */

} // namespace r1
} // namespace detail
} // namespace tbb
