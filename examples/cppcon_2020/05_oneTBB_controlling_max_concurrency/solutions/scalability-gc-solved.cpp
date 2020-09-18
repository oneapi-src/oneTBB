//==============================================================
// Copyright (c) 2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// =============================================================

#include <chrono>
#include <iostream>
#include <tbb/tbb.h>

#include "../common/test_function.h"

#define INCORRECT_VALUE hw_threads

int main() {
  const int hw_threads = std::thread::hardware_concurrency();
  for (int i = 1; i <= 2*hw_threads; ++i) {
    // STEP A: Use i instead of INCORRECT_VALUE to vary the value from 1 to 2*hw_threads
    tbb::global_control gc(tbb::global_control::max_allowed_parallelism, i);
    run_test(); // warm-up run
    auto t0 = std::chrono::high_resolution_clock::now();
    auto num_participating_threads = run_test(); // test run
    auto sec = 1e-9*(std::chrono::high_resolution_clock::now() - t0).count();
    std::cout << "Ran test with on hw with " << hw_threads << " threads using "
              << num_participating_threads << " threads. Time == " << sec << " seconds." << std::endl
              << "1/" << hw_threads << " == " << 1.0/hw_threads << std::endl;
  }
}
