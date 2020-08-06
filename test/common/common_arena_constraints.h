/*
    Copyright (c) 2019-2020 Intel Corporation

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

#define TBB_PREVIEW_NUMA_SUPPORT 1
#include "tbb/detail/_config.h"

#include "common/test.h"
#include "common/utils_concurrency_limit.h"

#include "tbb/task_arena.h"

#include <vector>

#if __TBB_HWLOC_PRESENT
#include "tbb/concurrent_unordered_set.h"

#include <atomic>

#define __TBB_EXTRA_DEBUG 1

#ifndef NUMBER_OF_PROCESSORS_GROUPS
#define NUMBER_OF_PROCESSORS_GROUPS 1
#endif

#if _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4100 )
#pragma warning( disable : 4996 )
#endif
#include <hwloc.h>
#if _MSC_VER
#pragma warning( pop )
#endif

#if _MSC_VER
#include <winbase.h>
#endif

//TODO: Write a test that checks for memory leaks during dynamic link/unlink of TBBbind.

bool is_system_environment_supported() {
#if _WIN32 && !_WIN64
    // HWLOC cannot proceed affinity masks on Windows in 32-bit mode if there are more than 32 logical CPU.
    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);
    if (si.dwNumberOfProcessors > 32) return false;
#endif // _WIN32 && !_WIN64
    return true;
}

// Macro to check hwloc interfaces return codes
#define hwloc_require_ex(command, ...)                                          \
        REQUIRE_MESSAGE(command(__VA_ARGS__) >= 0, "Error occurred inside hwloc call.");

namespace numa_validation {
    namespace {
        class system_info_t {
            hwloc_topology_t topology;

            hwloc_nodeset_t process_node_set;
            hwloc_cpuset_t  process_cpu_set;

            hwloc_cpuset_t buffer_cpu_set;
            hwloc_cpuset_t buffer_node_set;

            // hwloc_cpuset_t, hwloc_nodeset_t (inherited from hwloc_bitmap_t ) is pointers,
            // so we must manage memory allocation and deallocation
            typedef tbb::concurrent_unordered_set<hwloc_bitmap_t> memory_handler_t;
            memory_handler_t memory_handler;

            bool is_initialized;
        public:
            system_info_t() : memory_handler() {
                is_initialized = false;
            }

            void  initialize() {
                if (is_initialized) return;

                hwloc_require_ex(hwloc_topology_init, &topology);
                hwloc_require_ex(hwloc_topology_load, topology);

#if _WIN32 || _WIN64
                if ( GetActiveProcessorGroupCount() > 1 ) {
                    process_cpu_set  = hwloc_bitmap_dup(hwloc_topology_get_complete_cpuset (topology));
                    process_node_set = hwloc_bitmap_dup(hwloc_topology_get_complete_nodeset(topology));
                } else {
#endif /*_WIN32 || _WIN64*/
                    process_cpu_set  = hwloc_bitmap_alloc();
                    process_node_set = hwloc_bitmap_alloc();

                    hwloc_require_ex(hwloc_get_cpubind, topology, process_cpu_set, 0);
                    hwloc_cpuset_to_nodeset(topology, process_cpu_set, process_node_set);
#if _WIN32 || _WIN64
                }
#endif /*_WIN32 || _WIN64*/

                // If system contains no NUMA nodes, HWLOC 1.11 returns an infinitely filled bitmap.
                // hwloc_bitmap_weight() returns negative value for such bitmaps, so we use this check
                // to workaround this case.
                if (hwloc_bitmap_weight(process_node_set) <= 0) {
                    hwloc_bitmap_only(process_node_set, 0);
                }

// Debug macros for test topology parser validation
#if NUMBER_OF_NUMA_NODES
                REQUIRE_MESSAGE(hwloc_bitmap_weight(process_node_set) == NUMBER_OF_NUMA_NODES,
                    "Manual NUMA nodes count check.");
#endif /*NUMBER_OF_NUMA_NODES*/

                buffer_cpu_set  = hwloc_bitmap_alloc();
                buffer_node_set = hwloc_bitmap_alloc();

                is_initialized = true;
            }

            ~system_info_t() {
                if (is_initialized) {
                    for (memory_handler_t::iterator it = memory_handler.begin();
                        it != memory_handler.end(); it++) {
                        hwloc_bitmap_free(*it);
                    }
                    hwloc_bitmap_free(process_cpu_set);
                    hwloc_bitmap_free(process_node_set);
                    hwloc_bitmap_free(buffer_cpu_set);
                    hwloc_bitmap_free(buffer_node_set);

                    hwloc_topology_destroy(topology);
                }
            }

            hwloc_bitmap_t allocate_empty_affinity_mask() {
                REQUIRE_MESSAGE(is_initialized, "Call of uninitialized system_info");
                hwloc_bitmap_t result = hwloc_bitmap_alloc();
                memory_handler.insert(result);
                return result;
            }

            hwloc_cpuset_t allocate_current_cpu_set() {
                REQUIRE_MESSAGE(is_initialized, "Call of uninitialized system_info");
                hwloc_cpuset_t current_affinity_mask = allocate_empty_affinity_mask();
                hwloc_require_ex(hwloc_get_cpubind, topology, current_affinity_mask, HWLOC_CPUBIND_THREAD );
                REQUIRE_MESSAGE(!hwloc_bitmap_iszero(current_affinity_mask), "Empty current affinity mask.");
                return current_affinity_mask;
            }

            hwloc_const_cpuset_t get_process_cpu_set() {
                REQUIRE_MESSAGE(is_initialized, "Call of uninitialized system_info");
                return process_cpu_set;
            }

            hwloc_const_nodeset_t get_process_node_set() {
                REQUIRE_MESSAGE(is_initialized, "Call of uninitialized system_info");
                return process_node_set;
            }

            int numa_node_max_concurrency(int index) {
                REQUIRE_MESSAGE(is_initialized, "Call of uninitialized system_info");
                hwloc_bitmap_only(buffer_node_set, index);
                hwloc_cpuset_from_nodeset(topology, buffer_cpu_set, buffer_node_set);
                hwloc_bitmap_and(buffer_cpu_set, buffer_cpu_set, process_cpu_set);
                REQUIRE_MESSAGE(hwloc_bitmap_weight(buffer_cpu_set) > 0, "Negative concurrency.");
                return hwloc_bitmap_weight(buffer_cpu_set);
            }
        };

        static system_info_t system_info;
    } /*internal namespace*/

typedef hwloc_bitmap_t affinity_mask;
typedef hwloc_const_bitmap_t const_affinity_mask;

void initialize_system_info() { system_info.initialize(); }

affinity_mask allocate_current_cpu_set() {
    return system_info.allocate_current_cpu_set();
}

bool affinity_masks_isequal(const_affinity_mask first, const_affinity_mask second) {
    return hwloc_bitmap_isequal(first, second) ? true : false;
}

bool affinity_masks_intersects(const_affinity_mask first, const_affinity_mask second) {
    return hwloc_bitmap_intersects(first, second) ? true : false;
}

void validate_topology_information(std::vector<tbb::numa_node_id> numa_indexes) {
    // Generate available numa nodes bitmap
    const_affinity_mask process_node_set = system_info.get_process_node_set();

    // Parse input indexes list to numa nodes bitmap
    affinity_mask merged_input_node_set = system_info.allocate_empty_affinity_mask();
    int whole_system_concurrency = 0;
    for (unsigned i = 0; i < numa_indexes.size(); i++) {
        REQUIRE_MESSAGE(!hwloc_bitmap_isset(merged_input_node_set, numa_indexes[i]), "Indices are repeated.");
        hwloc_bitmap_set(merged_input_node_set, numa_indexes[i]);

        REQUIRE_MESSAGE(tbb::info::default_concurrency(numa_indexes[i]) ==
            system_info.numa_node_max_concurrency(numa_indexes[i]),
            "Wrong default concurrency value.");
        whole_system_concurrency += tbb::info::default_concurrency(numa_indexes[i]);
    }

    REQUIRE_MESSAGE(whole_system_concurrency == utils::get_platform_max_threads(),
           "Wrong whole system default concurrency level.");
    REQUIRE_MESSAGE(affinity_masks_isequal(process_node_set, merged_input_node_set),
           "Input array of indices is not equal with process numa node set.");
}

    template <typename It>
    typename std::enable_if<std::is_same<typename std::iterator_traits<It>::value_type, affinity_mask>::value, void>::
    type affinity_set_verification(It begin, It end) {
        affinity_mask buffer_mask = system_info.allocate_empty_affinity_mask();
        for (auto it = begin; it != end; it++) {
            REQUIRE_MESSAGE(!hwloc_bitmap_intersects(buffer_mask, *it),
                   "Bitmaps that are binded to different nodes are intersects.");
            // Add masks to buffer_mask to concatenate process affinity mask
            hwloc_bitmap_or(buffer_mask, buffer_mask,  *it);
        }

        REQUIRE_MESSAGE(affinity_masks_isequal(system_info.get_process_cpu_set(), buffer_mask),
               "Some cores was not included to bitmaps.");
    }
} /*namespace numa_validation*/

#endif /*__TBB_HWLOC_PRESENT*/
