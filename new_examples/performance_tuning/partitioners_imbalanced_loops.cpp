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

#include <iostream>
#include <tbb/tbb.h>

void doWork(double sec);

template <typename Partitioner>
void buildingWork(int N, const Partitioner& p) {
  tbb::parallel_for( tbb::blocked_range<int>(0, N, 1), 
    [](const tbb::blocked_range<int>& r) {
      for (int i = r.begin(); i < r.end(); ++i) {
        doWork(i);
      }
    }, p
  );
}

template <typename Partitioner>
void buildingWork(int N, Partitioner& p) {
  tbb::parallel_for( tbb::blocked_range<int>(0, N, 1), 
    [](const tbb::blocked_range<int>& r) {
      for (int i = r.begin(); i <  r.end(); ++i) {
        doWork(i);
      }
    }, p
  );
}

static void warmupTBB() {
  // This is a simple loop that should get workers started.
  // oneTBB creates workers lazily on first use of the library
  // so this hides the startup time when looking at trivial
  // examples that do little real work. 
  tbb::parallel_for(0, tbb::info::default_concurrency(), 
    [=](int) {
      tbb::tick_count t0 = tbb::tick_count::now();
      while ((tbb::tick_count::now() - t0).seconds() < 0.01);
    }
  );
}

void doWork(double usec) {
  double sec = usec*1e-06;
  tbb::tick_count t0 = tbb::tick_count::now();
  while ((tbb::tick_count::now() - t0).seconds() <= sec);
}

int main(int argc, char *argv[]) {
  // use the most performance codes
  // only a single NUMA node
  // and only 1 thread per core
  tbb::task_arena::constraints c;
  c.set_numa_id(tbb::info::numa_nodes()[0]);
  c.set_core_type(tbb::info::core_types().back());
  c.set_max_threads_per_core(1);
  c.set_max_concurrency(std::min(8, tbb::info::default_concurrency(c)));
  tbb::task_arena a(c);

  std::cout << "Using an arena with " << a.max_concurrency() << " slots\n";

  a.execute([&]() {
    int N = 1000;
    int M = 10;

    std::cout << std::endl << "M = " << M
              << std::endl << "N = " << N << std::endl;

    warmupTBB();
    tbb::tick_count t0 = tbb::tick_count::now();
    for (int i = 0; i < M; ++i) {
      buildingWork(N, tbb::auto_partitioner{});
    }
    double auto_time = (tbb::tick_count::now() - t0).seconds();

    warmupTBB();
    tbb::affinity_partitioner aff_p;
    t0 = tbb::tick_count::now();
    for (int i = 0; i < M; ++i) {
      buildingWork(N, aff_p); 
    }
    double affinity_time = (tbb::tick_count::now() - t0).seconds();

    warmupTBB();
    t0 = tbb::tick_count::now();
    for (int i = 0; i < M; ++i) {
      buildingWork(N, tbb::static_partitioner{});
    }
    double static_time = (tbb::tick_count::now() - t0).seconds();

    std::cout << "auto_partitioner = " << auto_time << " seconds" << std::endl
              << "affinity_partitioner = " << affinity_time << " seconds" << std::endl
              << "static_partitioner = " << static_time << " seconds" << std::endl;
  });
  return 0;
}

