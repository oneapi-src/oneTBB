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

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <tbb/tbb.h>

template <typename Partitioner>
void parForAdd(double v, int N, double *a, const Partitioner& p) {
  tbb::parallel_for( tbb::blocked_range<int>(0, N, 1), 
    [v, a](const tbb::blocked_range<int>& r) {
      for (int i = r.begin(); i < r.end(); ++i) {
        a[i] += v;
      }
    }, p
  );
}

template <typename Partitioner>
void parForAdd(double v, int N, double *a, Partitioner& p) {
  tbb::parallel_for( tbb::blocked_range<int>(0, N, 1), 
    [v, a](const tbb::blocked_range<int>& r) {
      for (int i = r.begin(); i < r.end(); ++i) {
        a[i] += v;
      }
    }, p
  );
}

void resetV(int N, double *v);
void resetA(int N, double *a);
static void warmupTBB();

int main(int argc, char *argv[]) {
  // use the most performance codes
  // only a single NUMA node
  // and only 1 thread per core
  tbb::task_arena::constraints c;
  c.set_numa_id(tbb::info::numa_nodes()[0]);
  c.set_core_type(tbb::info::core_types().back());
  c.set_max_threads_per_core(1);
  c.set_max_concurrency(std::min(8, tbb::info::default_concurrency(c)));
  tbb::task_arena arena(c);

  std::cout << "Using an arena with " << arena.max_concurrency() << " slots\n";

  arena.execute([&]() {
    int M = 10000;
    int N = 100000;

    std::cout << "P = " << tbb::info::default_concurrency()
              << std::endl << "N = " << N 
              << std::endl << "M = " << M << std::endl;

    double *v = new double[M];
    double *a = new double[N]; 

    warmupTBB();
    resetV(M, v);
    resetA(N, a);
    tbb::tick_count t0 = tbb::tick_count::now();
    for (int i = 0; i < M; ++i) {
      parForAdd(v[i], N, a, tbb::auto_partitioner{});
    }
    double auto_time = (tbb::tick_count::now() - t0).seconds();

    warmupTBB();
    resetA(N, a);
    tbb::affinity_partitioner aff_p;
    t0 = tbb::tick_count::now();
    for (int i = 0; i < M; ++i) {
      parForAdd(v[i], N, a, aff_p); 
    }
    double affinity_time = (tbb::tick_count::now() - t0).seconds();

    warmupTBB();
    resetA(N, a);
    t0 = tbb::tick_count::now();
    for (int i = 0; i < M; ++i) {
      parForAdd(v[i], N, a, tbb::static_partitioner{});
    }
    double static_time = (tbb::tick_count::now() - t0).seconds();

    std::cout << "auto_partitioner = " << auto_time << std::endl
              << "affinity_partitioner = " << affinity_time << std::endl
              << "static_partitioner = " << static_time << std::endl;

    delete [] v;
    delete [] a;
  });

  return 0;
}


void resetV(int N, double *v) {
  for (int i = 0; i < N; ++i) {
    v[i] = i;
  }
  std::shuffle(v, v+N, std::random_device{});
}

void resetA(int N, double *a) {
  for (int i = 0; i < N; ++i) {
    a[i] = 0;
  }
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