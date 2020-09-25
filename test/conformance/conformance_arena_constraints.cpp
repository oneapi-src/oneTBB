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

//! \file conformance_arena_constraints.cpp
//! \brief Test for [preview] functionality

#include "common/common_arena_constraints.h"

#if __TBB_HWLOC_PRESENT

struct execute_wrapper {
    template <typename Callable>
    void emplace_function(tbb::task_arena& ta, Callable functor) {
        ta.execute(functor);
    }
};

struct enqueue_wrapper {
    template <typename Callable>
    void emplace_function(tbb::task_arena& ta, Callable functor) {
        ta.enqueue(functor);
    }
};

template <typename It, typename FuncWrapper>
typename std::enable_if<std::is_same<typename std::iterator_traits<It>::value_type, tbb::task_arena>::value, void>::
type test_numa_binding_impl(It begin, It end, FuncWrapper wrapper) {
    tbb::concurrent_unordered_set<numa_validation::affinity_mask> affinity_masks;
    std::atomic<unsigned> counter(0), expected_count(0);

    auto affinity_mask_checker = [&counter, &affinity_masks]() {
        affinity_masks.insert(numa_validation::allocate_current_cpu_set());
        counter++;
    };

    for (auto it = begin; it != end; it++) {
        expected_count++;
        wrapper.emplace_function(*it, affinity_mask_checker);
    }

    // Wait for all spawned tasks
    while (counter != expected_count) {}
    numa_validation::affinity_set_verification(affinity_masks.begin(),affinity_masks.end());
}

//! Testing that arenas bind to NUMA nodes correctly
//! \brief \ref interface \ref requirement
TEST_CASE("Test binding to NUMA nodes correctness") {
    if (is_system_environment_supported()) {
        numa_validation::initialize_system_info();
        std::vector<int> numa_indexes = tbb::info::numa_nodes();
        std::vector<tbb::task_arena> arenas(numa_indexes.size());

        for(unsigned i = 0; i < numa_indexes.size(); i++) {
            // Bind arenas to numa nodes
            arenas[i].initialize(tbb::task_arena::constraints(numa_indexes[i]));
        }

        test_numa_binding_impl(arenas.begin(), arenas.end(), execute_wrapper());
        test_numa_binding_impl(arenas.begin(), arenas.end(), enqueue_wrapper());
    }
}

#else /*__TBB_HWLOC_PRESENT*/

//! Testing NUMA support interfaces validity when HWLOC is not presented on system
//! \brief \ref interface \ref requirement
TEST_CASE("Test NUMA support interfaces validity when HWLOC is not presented on system") {
    std::vector<int> numa_indexes = tbb::info::numa_nodes();

    REQUIRE_MESSAGE(numa_indexes.size() == 1,
        "Number of NUMA nodes must be pinned to 1, if we have no HWLOC on the system.");
    REQUIRE_MESSAGE(numa_indexes[0] == -1,
        "Index of NUMA node must be pinned to -1, if we have no HWLOC on the system.");
    REQUIRE_MESSAGE(  tbb::info::default_concurrency(numa_indexes[0]) == utils::get_platform_max_threads(),
        "Concurrency for NUMA node must be equal to default_num_threads(), if we have no HWLOC on the system.");
}

#endif /*__TBB_HWLOC_PRESENT*/
