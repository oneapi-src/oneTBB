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
#include <math.h>
#include <tbb/tbb.h>

double serialPiExample(int num_intervals) {
  double dx = 1.0 / num_intervals;
  double sum = 0.0;
  for (int i = 0; i < num_intervals; ++i) {
    double x = (i+0.5)*dx;
    double h = sqrt(1-x*x);
    sum += h*dx;
  }
  return 4 * sum;
}

template< typename Partitioner >
double reducePiExample(int num_intervals, int grainsize) {
  double dx = 1.0 / num_intervals;
  double sum = tbb::parallel_reduce(
    /* range = */ tbb::blocked_range<int>(0, num_intervals, grainsize), 
    /* idenity = */ 0.0,
    /* func */ 
    [=](const tbb::blocked_range<int>& r, double init) -> double {
      for (int i = r.begin(); i != r.end(); ++i) {
        double x = (i + 0.5)*dx;
        double h = sqrt(1 - x*x);
        init += h*dx;
      }
      return init;
    },
    /* reduction */
    [](double x, double y) -> double {
        return x + y;
    }, 
    /* partitioner */ Partitioner()
  );
  return 4 * sum;
}

template< typename Partitioner >
double deterministicReducePiExample(int num_intervals, int grainsize) {
  double dx = 1.0 / num_intervals;
  double sum = tbb::parallel_deterministic_reduce(
    /* range = */ tbb::blocked_range<int>(0, num_intervals, grainsize), 
    /* identity = */ 0.0,
    /* func */ 
    [=](const tbb::blocked_range<int>& r, double init) -> double {
      for (int i = r.begin(); i != r.end(); ++i) {
        double x = (i + 0.5)*dx;
        double h = sqrt(1 - x*x);
        init += h*dx;
      }
      return init;
    },
    /* reduction */
    [](double x, double y) -> double {
      return x + y;
    },
    /* partitioner */ Partitioner()
  );
  return 4 * sum;
}

static void warmupTBB() {
  tbb::parallel_for(0, tbb::info::default_concurrency(), [](int) {
    tbb::tick_count t0 = tbb::tick_count::now();
    while ((tbb::tick_count::now() - t0).seconds() < 0.01);
  });
}

int main() {
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
    int num_intervals = 1<<26;
    tbb::tick_count ts_0 = tbb::tick_count::now();
    double spi = serialPiExample(num_intervals);
    tbb::tick_count ts_1 = tbb::tick_count::now();
    double serial_time = (ts_1 - ts_0).seconds();
    std::cout << "serial, " << spi << ", " << serial_time << std::endl;
    warmupTBB();
    std::cout << "speedups relative to serial:" << std::endl;
    std::cout << "gs, r-simple, d-simple, r-static, d-static, r-auto" << std::endl;
    for (int gs = 1; gs <= num_intervals; gs *= 2) {
        reducePiExample<tbb::auto_partitioner>(num_intervals, gs);
        tbb::tick_count t0 = tbb::tick_count::now();
        double v0 = reducePiExample<tbb::auto_partitioner>(num_intervals, gs);
        tbb::tick_count t1 = tbb::tick_count::now();
        double v1 = reducePiExample<tbb::simple_partitioner>(num_intervals, gs);
        tbb::tick_count t2 = tbb::tick_count::now();
        double v2 = reducePiExample<tbb::static_partitioner>(num_intervals, gs);
        tbb::tick_count t3 = tbb::tick_count::now();
        double v3 = deterministicReducePiExample<tbb::simple_partitioner>(num_intervals, gs);
        tbb::tick_count t4 = tbb::tick_count::now();
        double v4 = deterministicReducePiExample<tbb::static_partitioner>(num_intervals, gs);
        tbb::tick_count t5 = tbb::tick_count::now();
        std::cout << v0 << ", " << v1 << ", " << v2 
                  << ", " << v3 << ", " << v4 << "\n";
        std::cout << gs 
                << ", " << serial_time / (t2-t1).seconds()
                << ", " << serial_time / (t4-t3).seconds()
                << ", " << serial_time / (t3-t2).seconds()
                << ", " << serial_time / (t5-t4).seconds() 
                << ", " << serial_time / (t1-t0).seconds()
                << std::endl;
    }
  });
  return 0;
}

