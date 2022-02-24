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

#ifndef _TBB_governor_H
#define _TBB_governor_H

#include "oneapi/tbb/mutex.h"
#include "rml_tbb.h"

#include "misc.h" // for AvailableHwConcurrency
#include "tls.h"

namespace tbb {
namespace detail {
namespace r1 {

class market;
class thread_data;
class __TBB_InitOnce;
class market_concurrent_monitor;
class permit_manager;
class thread_dispatcher;

#if __TBB_USE_ITT_NOTIFY
//! Defined in profiling.cpp
extern bool ITT_Present;
#endif

typedef std::size_t stack_size_type;

// class threading_lifetime_controller {
// public:
//     using global_mutex_type = mutex;

//     static bool add_unsafe_reference(global_mutex_type::scoped_lock& lock, bool is_public, unsigned max_num_workers = 0, std::size_t stack_size = 0) {
//         threading_lifetime_controller *tlc = theTLC;
//         if (tlc) {
//             ++tlc->my_ref_count;
//             const unsigned old_public_count = is_public ? tlc->my_public_ref_count++ : /*any non-zero value*/ 1;
//             lock.release();
//             if (old_public_count == 0)
//                 set_active_num_workers(calc_workers_soft_limit(workers_requested, tlc->my_num_workers_hard_limit));

//             // do not warn if default number of workers is requested
//             if (workers_requested != governor::default_num_threads() - 1)  {
//                 __TBB_ASSERT(skip_soft_limit_warning > workers_requested,
//                              "skip_soft_limit_warning must be larger than any valid workers_requested");
//                 unsigned soft_limit_to_report = tlc->my_workers_soft_limit_to_report.load(std::memory_order_relaxed);
//                 if (soft_limit_to_report < workers_requested) {
//                     runtime_warning("The number of workers is currently limited to %u. "
//                                     "The request for %u workers is ignored. Further requests for more workers "
//                                     "will be silently ignored until the limit changes.\n",
//                                     soft_limit_to_report, workers_requested);
//                     // The race is possible when multiple threads report warnings.
//                     // We are OK with that, as there are just multiple warnings.
//                     unsigned expected_limit = soft_limit_to_report;
//                     tlc->my_workers_soft_limit_to_report.compare_exchange_strong(expected_limit, skip_soft_limit_warning);
//                 }
//             }
//             if (tlc->my_stack_size < stack_size)
//                 runtime_warning("Thread stack size has been already set to %u. "
//                                 "The request for larger stack (%u) cannot be satisfied.\n",
//                                 tlc->my_stack_size, stack_size);
//             return true;
//         }
//         return false;
//     }

//     void get_permit_manager() {

//     }

//     void get_thread_pool() {

//     }

//     void release() {

//     }

// private:
//     static threading_lifetime_controller* theTLC;

//     //! ABA prevention marker to assign to newly created arenas
//     std::atomic<uintptr_t> my_arenas_aba_epoch{0};

//     //! Reference count controlling market object lifetime
//     std::atomic<unsigned> my_ref_count{0};

//     //! Count of external threads attached
//     std::atomic<unsigned> my_public_ref_count{0};

//     permit_manager* my_permit_manager;
//     thread_dispatcher* my_thread_dispatcher;

//     //! Mutex guarding creation/destruction of theMarket, insertions/deletions in my_arenas, and cancellation propagation
//     static global_mutex_type theTLCMutex;
// };

//------------------------------------------------------------------------
// Class governor
//------------------------------------------------------------------------

//! The class handles access to the single instance of market, and to TLS to keep scheduler instances.
/** It also supports automatic on-demand initialization of the TBB scheduler.
    The class contains only static data members and methods.*/
class governor {
private:
    friend class __TBB_InitOnce;
    friend class market;
    friend class thread_dispatcher;

    // TODO: consider using thread_local (measure performance and side effects)
    //! TLS for scheduler instances associated with individual threads
    static basic_tls<thread_data*> theTLS;

    // TODO (TBB_REVAMP_TODO): reconsider constant names
    static rml::tbb_factory theRMLServerFactory;

    static bool UsePrivateRML;

    // Flags for runtime-specific conditions
    static cpu_features_type cpu_features;
    static bool is_rethrow_broken;

    static market_concurrent_monitor* sleep_monitor;

    //! Create key for thread-local storage and initialize RML.
    static void acquire_resources ();

    //! Destroy the thread-local storage key and deinitialize RML.
    static void release_resources ();

    static rml::tbb_server* create_rml_server ( rml::tbb_client& );

public:
    static unsigned default_num_threads () {
        // Caches the maximal level of parallelism supported by the hardware
        static unsigned num_threads = AvailableHwConcurrency();
        return num_threads;
    }
    static std::size_t default_page_size () {
        // Caches the size of OS regular memory page
        static std::size_t page_size = DefaultSystemPageSize();
        return page_size;
    }
    static void one_time_init();
    //! Processes scheduler initialization request (possibly nested) in an external thread
    /** If necessary creates new instance of arena and/or local scheduler.
        The auto_init argument specifies if the call is due to automatic initialization. **/
    static void init_external_thread();

    //! The routine to undo automatic initialization.
    /** The signature is written with void* so that the routine
        can be the destructor argument to pthread_key_create. */
    static void auto_terminate(void* tls);

    //! Obtain the thread-local instance of the thread data.
    /** If the scheduler has not been initialized yet, initialization is done automatically.
        Note that auto-initialized scheduler instance is destroyed only when its thread terminates. **/
    static thread_data* get_thread_data() {
        thread_data* td = theTLS.get();
        if (td) {
            return td;
        }
        init_external_thread();
        td = theTLS.get();
        __TBB_ASSERT(td, nullptr);
        return td;
    }

    static void set_thread_data(thread_data& td) {
        theTLS.set(&td);
    }

    static void clear_thread_data() {
        theTLS.set(nullptr);
    }

    static thread_data* get_thread_data_if_initialized () {
        return theTLS.get();
    }

    static bool is_thread_data_set(thread_data* td) {
        return theTLS.get() == td;
    }

    //! Undo automatic initialization if necessary; call when a thread exits.
    static void terminate_external_thread() {
        auto_terminate(get_thread_data_if_initialized());
    }

    static void initialize_rml_factory ();

    static bool does_client_join_workers (const rml::tbb_client &client);

    static bool speculation_enabled() { return cpu_features.rtm_enabled; }

#if __TBB_WAITPKG_INTRINSICS_PRESENT
    static bool wait_package_enabled() { return cpu_features.waitpkg_enabled; }
#endif

    static bool rethrow_exception_broken() { return is_rethrow_broken; }

    static bool is_itt_present() {
#if __TBB_USE_ITT_NOTIFY
        return ITT_Present;
#else
        return false;
#endif
    }

    //! Factory method creating new market object
    static permit_manager& get_permit_manager( bool is_public, unsigned max_num_workers = 0, std::size_t stack_size = 0 );

    //! Return wait list
    static market_concurrent_monitor& get_wait_list() { 
        return *sleep_monitor;
    }
}; // class governor

} // namespace r1
} // namespace detail
} // namespace tbb

#endif /* _TBB_governor_H */
