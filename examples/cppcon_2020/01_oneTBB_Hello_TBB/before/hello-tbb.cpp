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
  std::cout << "Running sequentially:\n";
  auto st0 = std::chrono::high_resolution_clock::now();
  sleep_then_print(2, "Hello");
  sleep_then_print(1, ", TBB!");
  auto st1 = std::chrono::high_resolution_clock::now();
    
  std::cout << "\nSerial time   = " << 1e-9 * (st1-st0).count() << " seconds\n";
}
