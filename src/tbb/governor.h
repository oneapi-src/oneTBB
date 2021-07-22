/*
    Copyright (c) 2005-2021 Intel Corporation

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

#include "rml_tbb.h"

#include "misc.h" // for AvailableHwConcurrency
#include "tls.h"

#if __TBB_STATISTICS
#include <mutex>
#include <iostream>
#include <iomanip>
#include <array>
#endif // __TBB_STATISTICS

namespace tbb {
namespace detail {
namespace r1 {

class market;
class thread_data;
class __TBB_InitOnce;

#if __TBB_USE_ITT_NOTIFY
//! Defined in profiling.cpp
extern bool ITT_Present;
#endif

typedef std::size_t stack_size_type;

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

    // TODO: consider using thread_local (measure performance and side effects)
    //! TLS for scheduler instances associated with individual threads
    static basic_tls<thread_data*> theTLS;

    //! Caches the maximal level of parallelism supported by the hardware
    static unsigned DefaultNumberOfThreads;

    //! Caches the size of OS regular memory page
    static std::size_t DefaultPageSize;

    // TODO (TBB_REVAMP_TODO): reconsider constant names
    static rml::tbb_factory theRMLServerFactory;

    static bool UsePrivateRML;

    // Flags for runtime-specific conditions
    static cpu_features_type cpu_features;
    static bool is_rethrow_broken;

    //! Create key for thread-local storage and initialize RML.
    static void acquire_resources ();

    //! Destroy the thread-local storage key and deinitialize RML.
    static void release_resources ();

    static rml::tbb_server* create_rml_server ( rml::tbb_client& );

public:
    static unsigned default_num_threads () {
        // No memory fence required, because at worst each invoking thread calls AvailableHwConcurrency once.
        return DefaultNumberOfThreads ? DefaultNumberOfThreads :
                                        DefaultNumberOfThreads = AvailableHwConcurrency();
    }
    static std::size_t default_page_size () {
        return DefaultPageSize ? DefaultPageSize :
                                 DefaultPageSize = DefaultSystemPageSize();
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
        __TBB_ASSERT(td, NULL);
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

#if __TBB_STATISTICS
    enum class statistics : std::size_t {
        exec_task = 0,
        get_task,
        task_res_not_null,
        steal_task,
        try_steal_task,
        victim_pool_empty,
        size,
    };
    using statistic_array = std::array<std::uint32_t, static_cast<std::size_t>(statistics::size)>;

    class collect_statistics : no_copy {
    public:
        collect_statistics() = default;

        ~collect_statistics() = default;

        void emplace(array_stat thread_stat) {
            std::lock_guard<std::mutex> lk {m_};
            accumulate_statistics.emplace_back(thread_stat);
        }

        // thread-unsafe method
        void print_statistics() {
                int offset = 2;
                /* std::string not so bad because of Small String Optimization */
                std::string column_names[] = {
                    "executed", "getted",
                    "result != null", "steal", "try steal",
                    "vic pool empty",
                };
                for (auto& el : column_names) {
                    std::cout << std::left << std::setw(el.size() + offset) << el;
                }
                std::cout << std::endl;
#if __TBB_STATISTICS_SUM
                array_stat summary{};
#endif // __TBB_STATISTICS_SUM
                for (const auto &thread_stat : accumulate_statistics) {
                    for (std::size_t k = 0; k < thread_stat.size(); ++k) {
                        std::cout << std::left << std::setw(column_names[k].size() + offset) << thread_stat[k];
#if __TBB_STATISTICS_SUM
                        summary[k] += thread_stat[k];
#endif // __TBB_STATISTICS_SUM
                    }
                    std::cout << std::endl;
                }
#if __TBB_STATISTICS_SUM
                std::cout << "SUMMARY:" << std::endl;
                for (std::size_t k = 0; k < summary.size(); ++k) {
                    std::cout << std::left << std::setw(column_names[k].size() + offset) << summary[k];
                }
                std::cout << std::endl;
#endif // __TBB_STATISTICS_SUM
        }
        std::mutex m_;
        std::vector<array_stat> accumulate_statistics {};
    };
    static collect_statistics accumulator;
#endif // __TBB_STATISTICS
}; // class governor

} // namespace r1
} // namespace detail
} // namespace tbb

#endif /* _TBB_governor_H */
