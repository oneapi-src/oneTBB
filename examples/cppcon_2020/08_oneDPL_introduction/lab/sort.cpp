//==============================================================
// Copyright (c) 2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// =============================================================

#include <chrono>
#include <iostream>
#include <vector>
#include <random>

#include <oneapi/dpl/execution>
#include <oneapi/dpl/algorithm>

int main() {
  std::default_random_engine e1(0);
  std::uniform_int_distribution<int> d(0, 5000);

  // initialize input vector with random numbers
  std::vector<int> data(5000000);
  for(auto& i:data) {
    i = d(e1);
  }

  {
    std::vector<int> input(data);
    std::cout << "Running sequentially:\n";
    auto st0 = std::chrono::high_resolution_clock::now();
    std::sort(input.begin(), input.end());
    auto st1 = std::chrono::high_resolution_clock::now();
    std::cout << "Serial time   = " << 1e-9 * (st1-st0).count() << " seconds\n";
  }

  {
    std::vector<int> input(data);
    std::cout << "\nRunning in parallel:\n";
    auto pt0 = std::chrono::high_resolution_clock::now();
    std::sort(std::execution::par, input.begin(), input.end());
    auto pt1 = std::chrono::high_resolution_clock::now();
    std::cout << "Parallel time = " << 1e-9 * (pt1-pt0).count() << " seconds\n";
  }
    
  return 0;
}
