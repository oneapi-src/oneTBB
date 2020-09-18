//==============================================================
// Copyright (c) 2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// =============================================================

#include <chrono>
#include <iostream>
#include <thread>
#include <tbb/tbb.h>

void sleep_then_print(unsigned sec, const char *str) {
  std::this_thread::sleep_for(std::chrono::seconds(sec));
  std::cout << str << std::flush;
}

int main() {
  std::cout << "\nRunning in parallel:\n";
  auto pt0 = std::chrono::high_resolution_clock::now();
  // STEP 1: remove the '//' at the start of the lines to use parallel_invoke
  //  tbb::parallel_invoke(
  //  []() { 
    sleep_then_print(2, "Hello"); 
  // },
  //  []() { 
    sleep_then_print(1, ", TBB!"); 
  // });
  auto pt1 = std::chrono::high_resolution_clock::now();
    
  std::cout << "Parallel time = " << 1e-9 * (pt1-pt0).count() << " seconds\n";
}
