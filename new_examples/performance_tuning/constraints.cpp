/*
    Copyright (c) 2024 Intel Corporation

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

#include "tbb/tbb.h"
#include <vector>

#define USE_ARENA_TRACE 1
#if USE_ARENA_TRACE
#include "arena_trace.h"
#endif

int N = 1000;
double w = 0.01;
double f(double v);

void constrain_for_numa_nodes() {
#if USE_ARENA_TRACE
    arena_tracer t{"numa_trace.json"};
#endif

    std::vector<tbb::numa_node_id> numa_nodes = tbb::info::numa_nodes();
    std::vector<tbb::task_arena> arenas(numa_nodes.size());
    std::vector<tbb::task_group> task_groups(numa_nodes.size());

    for (int i = 0; i < numa_nodes.size(); i++) {
        arenas[i].initialize(tbb::task_arena::constraints(numa_nodes[i]), 0);
        #if USE_ARENA_TRACE
        t.add_arena(std::to_string(i), arenas[i]);
        #endif
    }
    for (int i = 0; i < numa_nodes.size(); i++) {
        arenas[i].execute([&task_groups, i] {
            task_groups[i].run([] {
                tbb::parallel_for(0, N, [](int j) { f(w); });
            });
        });
    }
    for (int i = 0; i < numa_nodes.size(); i++) {
        arenas[i].execute([&task_groups, i] {
            task_groups[i].wait();
        });
    }
}

void constrain_for_core_type() {

    std::vector<tbb::core_type_id> core_types = tbb::info::core_types();
    tbb::task_arena arena(
      tbb::task_arena::constraints{}.set_core_type(core_types.back())
    );

    #if USE_ARENA_TRACE
      arena_tracer t{"core_trace.json"};
      t.add_arena("pcores", arena);
    #endif

    arena.execute([] {
        tbb::parallel_for(0, N, [](int) { f(w); });
    });
}

void constrain_for_no_hyperthreading() {
    tbb::task_arena::constraints c;
    std::vector<tbb::core_type_id> core_types = tbb::info::core_types();
    c.set_core_type(core_types.back());
    c.set_max_threads_per_core(1);
    tbb::task_arena no_ht_arena(c);

    #if USE_ARENA_TRACE
      arena_tracer t{"no_ht_constraints_trace.json"};
      t.add_arena("no_ht_arena", no_ht_arena);
    #endif

    no_ht_arena.execute( [] {
        tbb::parallel_for(0, N, [](int) { f(w); });
    });
}

void limit_concurrency_for_no_hyperthreading() {
    tbb::task_arena::constraints c;
    std::vector<tbb::core_type_id> core_types = tbb::info::core_types();
    c.set_core_type(core_types.back());
    c.set_max_threads_per_core(1);
    int no_ht_concurrency = tbb::info::default_concurrency(c);
    tbb::task_arena arena( no_ht_concurrency );

    #if USE_ARENA_TRACE
      arena_tracer t{"no_ht_concurrency_trace.json"};
      t.add_arena("no_ht_concurrency", arena);
    #endif

    arena.execute( [] {
        tbb::parallel_for(0, N, [](int) { f(w); });
    });
}

int main() {
  constrain_for_numa_nodes();
  constrain_for_core_type();
  constrain_for_no_hyperthreading();
  limit_concurrency_for_no_hyperthreading();
  return 0;
}

double f(double v) {
  tbb::tick_count t0 = tbb::tick_count::now();
  while ((tbb::tick_count::now() - t0).seconds() < 0.01);
  return 2*v;
}
