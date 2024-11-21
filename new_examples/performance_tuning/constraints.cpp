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

#include <vector>
#include <tbb/tbb.h>


int N = 1000;
double w = 0.01;
double f(double v);

void constrain_for_numa_nodes() {
  std::vector<tbb::numa_node_id> numa_nodes = tbb::info::numa_nodes();
  std::vector<tbb::task_arena> arenas(numa_nodes.size());
  std::vector<tbb::task_group> task_groups(numa_nodes.size());

  // initialize each arena, each constrained to a different NUMA node
  for (int i = 0; i < numa_nodes.size(); i++)
    arenas[i].initialize(tbb::task_arena::constraints(numa_nodes[i]), 0);

  // enqueue work to all but the first arena, using the task_group to track work
  // by using defer, the task_group reference count is incremented immediately
  for (int i = 1; i < numa_nodes.size(); i++)
    arenas[i].enqueue(
      task_groups[i].defer([] { 
        tbb::parallel_for(0, N, [](int j) { f(w); }); 
      })
    );

  // directly execute the work to completion in the remaining arena
  arenas[0].execute([] {
    tbb::parallel_for(0, N, [](int j) { f(w); });
  });

  // join the other arenas to wait on their task_groups
  for (int i = 1; i < numa_nodes.size(); i++)
    arenas[i].execute([&task_groups, i] { task_groups[i].wait(); });
}

void constrain_for_core_type() {
    std::vector<tbb::core_type_id> core_types = tbb::info::core_types();
    tbb::task_arena arena(
      tbb::task_arena::constraints{}.set_core_type(core_types.back())
    );

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

    arena.execute( [] {
        tbb::parallel_for(0, N, [](int) { f(w); });
    });
}

#include <iostream>

int main() {
  std::cout << "Running numa node constraint example\n";
  constrain_for_numa_nodes();
  std::cout << "Running core type constraint example\n";
  constrain_for_core_type();
  std::cout << "Running one thread per core constraint example\n";
  constrain_for_no_hyperthreading();
    std::cout << "Running limited concurrency example\n";
  limit_concurrency_for_no_hyperthreading();
  std::cout << "done\n";
  return 0;
}

double f(double v) {
  tbb::tick_count t0 = tbb::tick_count::now();
  while ((tbb::tick_count::now() - t0).seconds() < 0.01);
  return 2*v;
}
