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

//! \file test_arena_constraints.cpp
//! \brief Test for [preview] functionality

#include "common/common_arena_constraints.h"
#include "common/spin_barrier.h"
#include "common/memory_usage.h"

#include "tbb/parallel_for.h"

#if __TBB_HWLOC_PRESENT
void recursive_arena_binding(int* numa_indexes, size_t count,
    std::vector<numa_validation::affinity_mask>& affinity_masks) {
    if (count > 0) {
        tbb::task_arena current_level_arena;
        current_level_arena.initialize(tbb::task_arena::constraints(numa_indexes[count - 1]));
        current_level_arena.execute(
            [&numa_indexes, &count, &affinity_masks]() {
                affinity_masks.push_back(numa_validation::allocate_current_cpu_set());
                recursive_arena_binding(numa_indexes, --count, affinity_masks);
            }
        );
    } else {
        // Validation of assigned affinity masks at the deepest recursion step
        numa_validation::affinity_set_verification(affinity_masks.begin(), affinity_masks.end());
    }

    if (!affinity_masks.empty()) {
        REQUIRE_MESSAGE(numa_validation::affinity_masks_isequal(affinity_masks.back(),
            numa_validation::allocate_current_cpu_set()),
            "After binding to different NUMA node thread affinity was not returned to previous state.");
        affinity_masks.pop_back();
    }
}

//! Testing binding correctness during passing through netsed arenas
//! \brief \ref interface \ref error_guessing
TEST_CASE("Test binding to NUMA nodes with nested arenas") {
    if (is_system_environment_supported()) {
        numa_validation::initialize_system_info();
        std::vector<int> numa_indexes = tbb::info::numa_nodes();
        std::vector<numa_validation::affinity_mask> affinity_masks;
        recursive_arena_binding(numa_indexes.data(), numa_indexes.size(), affinity_masks);
    }
}

//! Testing constraints propagation during arenas copy construction
//! \brief \ref regression
TEST_CASE("Test constraints propagation during arenas copy construction") {
    if (is_system_environment_supported()) {
        numa_validation::initialize_system_info();
        std::vector<int> numa_indexes = tbb::info::numa_nodes();
        for (auto index: numa_indexes) {
            numa_validation::affinity_mask constructed_mask, copied_mask;

            tbb::task_arena constructed{tbb::task_arena::constraints(index)};
            constructed.execute([&constructed_mask]() {
                constructed_mask = numa_validation::allocate_current_cpu_set();
            });

            tbb::task_arena copied(constructed);
            copied.execute([&copied_mask]() {
                copied_mask = numa_validation::allocate_current_cpu_set();
            });

            REQUIRE_MESSAGE(numa_validation::affinity_masks_isequal(constructed_mask, copied_mask),
                        "Affinity mask brokes during copy construction");
        }
    }
}
#endif /*__TBB_HWLOC_PRESENT*/

void collect_all_threads_on_barrier() {
    utils::SpinBarrier barrier;
    barrier.initialize(tbb::this_task_arena::max_concurrency());
    tbb::parallel_for(tbb::blocked_range<size_t>(0, tbb::this_task_arena::max_concurrency()),
        [&barrier](const tbb::blocked_range<size_t>&) {
            barrier.wait();
        });
};

//! Testing memory leaks absence
//! \brief \ref resource_usage
TEST_CASE("Test memory leaks") {
    std::vector<int> numa_indexes = tbb::info::numa_nodes();
    size_t num_traits = 1000;
    size_t current_memory_usage = 0, previous_memory_usage = 0, stability_counter = 0;
    bool no_memory_leak = false;

    for (size_t i = 0; i < num_traits; i++) {
        { /* All DTORs must be called before GetMemoryUsage() call*/
            std::vector<tbb::task_arena> arenas(numa_indexes.size());
            std::vector<tbb::task_group> task_groups(numa_indexes.size());

            for(unsigned j = 0; j < numa_indexes.size(); j++) {
                arenas[j].initialize(tbb::task_arena::constraints(numa_indexes[j]));
                arenas[j].execute(
                    [&task_groups, &j](){
                        task_groups[j].run([](){
                            collect_all_threads_on_barrier();
                        });
                    });
            }

            for(unsigned j = 0; j < numa_indexes.size(); j++) {
                arenas[j].execute([&task_groups, &j](){ task_groups[j].wait(); });
            }
        }

        current_memory_usage = utils::GetMemoryUsage();
        stability_counter = current_memory_usage==previous_memory_usage ? stability_counter + 1 : 0;
        // If the amount of used memory has not changed during 10% of executions,
        // then we can assume that the check was successful
        if (stability_counter > num_traits / 10) {
            no_memory_leak = true;
            break;
        }
        previous_memory_usage = current_memory_usage;
    }
    REQUIRE_MESSAGE(no_memory_leak, "Seems we get memory leak here.");
}
