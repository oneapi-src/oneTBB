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

#include <cmath>
#include <tbb/tbb.h>

//
// Estimating pi using numerical integration
//
double serialPI(int num_intervals) {
  double dx = 1.0 / num_intervals;
  double sum = 0.0;
  for (int i = 0; i < num_intervals; ++i) {
    double x = (i+0.5)*dx;
    double h = std::sqrt(1-x*x);
    sum += h*dx;
  }
  double pi = 4 * sum;
  return pi;
}

//
// Estimating pi using numerical integration
// with a TBB parallel_reduce
//
double parallelPI(int num_intervals) {
  double dx = 1.0 / num_intervals;
  double sum = tbb::parallel_reduce(
    /* range = */ tbb::blocked_range<int>(0, num_intervals), 
    /* idenity = */ 0.0,
    /* func */ 
    [=](const tbb::blocked_range<int>& r, double init) -> double {
      for (int i = r.begin(); i != r.end(); ++i) {
        double x = (i + 0.5)*dx;
        double h = std::sqrt(1 - x*x);
        init += h*dx;
      }
      return init;
    },
    /* reduction */
    [](double x, double y) -> double {
      return x + y;
    }
  );
  double pi = 4 * sum;
  return pi;
}

#include <iostream>
#include <limits>

static void warmupTBB();

int main() {
  const int num_intervals = std::numeric_limits<int>::max();

  tbb::tick_count t0 = tbb::tick_count::now();
  double serial_pi = serialPI(num_intervals);
  double serial_time = (tbb::tick_count::now() - t0).seconds();

  warmupTBB();
  tbb::tick_count t1 = tbb::tick_count::now();
  double tbb_pi = parallelPI(num_intervals);
  double tbb_time = (tbb::tick_count::now() - t1).seconds();

  std::cout << "serial_pi == " << serial_pi << std::endl;
  std::cout << "serial_time == " << serial_time << " seconds" << std::endl;
  std::cout << "tbb_pi == " << tbb_pi << std::endl;
  std::cout << "tbb_time == " << tbb_time << " seconds" << std::endl;
  std::cout << "speedup == " << serial_time / tbb_time << std::endl;
  return 0;
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

