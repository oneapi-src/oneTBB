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

#define INCORRECT_VALUE 1
#define INCORRECT_FUNCTION std::minus<double>()

double calc_pi(int num_intervals) {
  double dx = 1.0 / num_intervals;
  double sum = tbb::parallel_reduce(
    /* STEP 1: fix the upper bound: */ tbb::blocked_range<int>(0, INCORRECT_VALUE), 
    /* STEP 2: provide a proper identity value for summation */ INCORRECT_VALUE,
    /* func */ 
    [=](const tbb::blocked_range<int>& r, double init) -> double {
      for (int i = r.begin(); i != r.end(); ++i) {
        // STEP 3: Add the loop body code:
        //         Hint: it will look a lot like the the sequential code.
        //               the returned value should be (init + the_partial_sum)
      }
      return init;
    },
    // STEP 4: provide the reduction function
    //         Hint, maybe std::plus<double>{}
    INCORRECT_FUNCTION
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
