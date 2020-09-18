//==============================================================
// Copyright (c) 2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// =============================================================

#include <chrono>
#include <iostream>
#include <vector>

#include <oneapi/dpl/iterator>
#include <oneapi/dpl/execution>
#include <oneapi/dpl/algorithm>
#include <oneapi/dpl/numeric>

auto exec = std::execution::seq;

int main() {
  const size_t N = 100000;
  const size_t runs = 10;
  double r2, r1, r;

  // initialize input vectors
  std::vector<double> a(N);
  std::vector<double> b(N);

  auto input = oneapi::dpl::counting_iterator<size_t>(0);
  std::for_each_n(std::execution::par_unseq, input, N, [&](size_t i){ a[i] = 1.0; b[i] = 2.0; });
 
  // First run inner product
  auto st0 = std::chrono::high_resolution_clock::now();
  for(int k = 0; k <= runs; ++k) {
    if(k==1) st0 = std::chrono::high_resolution_clock::now();
    r1 = std::inner_product(a.begin(), a.end(), b.begin(), 0.0);
  }
  auto st1 = std::chrono::high_resolution_clock::now();

  // Second run parallel transform reduce
  auto pt0 = std::chrono::high_resolution_clock::now();
  for(int k = 0; k <= runs; ++k) {
    if(k==1) pt0 = std::chrono::high_resolution_clock::now();
    // *** Step 1: Replace the first parameter by execution policy par.
    r = std::transform_reduce(std::execution::par,
                              a.begin(), a.end(),
                              b.begin(), 0.0);
  }
  auto pt1 = std::chrono::high_resolution_clock::now();
  
  // Third run parallel and unsequenced transform reduce
  auto pt2 = std::chrono::high_resolution_clock::now();
  for(int k = 0; k <= runs; ++k) {
    if(k==1) pt2 = std::chrono::high_resolution_clock::now();
    // *** Step 2: Replace the first parameter by execution policy par_unseq.
    r2 = std::transform_reduce(std::execution::par_unseq,
                              a.begin(), a.end(),
                              b.begin(), 0.0);
  }
  auto pt3 = std::chrono::high_resolution_clock::now();

  std::cout << "Serial time    = " << 1e-6 * (st1-st0).count() / runs << " milliseconds\n";
  std::cout << "Parallel time  = " << 1e-6 * (pt1-pt0).count() / runs << " milliseconds\n"; 
  std::cout << "Par_unseq time = " << 1e-6 * (pt3-pt2).count() / runs << " milliseconds\n";
    
  std::cout << "Result: " << r << ", " << r1 << ", " << r2 << "\n";
    
  return 0;
}
