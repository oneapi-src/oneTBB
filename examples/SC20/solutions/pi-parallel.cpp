//==============================================================
// Copyright (c) 2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// =============================================================

#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <thread>
#include <tbb/tbb.h>

double calc_pi(int num_intervals) {
  double dx = 1.0 / num_intervals;
  double sum = tbb::parallel_reduce(
    /* range = */ tbb::blocked_range<int>(0, num_intervals ), 
    /* identity = */ 0.0,
    /* func */ 
    [=](const tbb::blocked_range<int>& r, double init) -> double {
      for (int i = r.begin(); i != r.end(); ++i) {
        double x = (i+0.5)*dx;
        double h = std::sqrt(1-x*x);
        init += h*dx;
      }
      return init;
    },
    std::plus<double>{}
  );
  double pi = 4 * sum;
  return pi;
}

static void warmupTBB() {
  int num_threads = std::thread::hardware_concurrency();
  tbb::parallel_for(0, num_threads,
    [](unsigned int) { 
      std::this_thread::sleep_for(std::chrono::milliseconds(10)); 
  });
}

int main() {
  const int num_intervals = std::numeric_limits<int>::max();
  double parallel_time = 0.0;
  warmupTBB();
  {
    auto pt0 = std::chrono::high_resolution_clock::now();
    double pi = calc_pi(num_intervals);
    parallel_time = 1e-9*(std::chrono::high_resolution_clock::now() - pt0).count();
    std::cout << "parallel pi == " << pi << std::endl;
  }

  std::cout << "parallel_time == " << parallel_time << " seconds" << std::endl;
  return 0;
}
