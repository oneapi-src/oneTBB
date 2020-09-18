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

int main() {

  {
    using oneapi::dpl::make_zip_iterator;
    std::vector<int> keys = { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1};
    std::vector<int> data = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto zip_in = make_zip_iterator(keys.begin(), data.begin());
    const size_t n = std::distance(keys.begin(), keys.end());  
    auto custom_func = [](auto l, auto r){ using std::get; return get<0>(l) < get<0>(r);};
      
    std::cout << "\nRunning in parallel:\n";
    auto pt0 = std::chrono::high_resolution_clock::now();
    std::sort(std::execution::par, zip_in, zip_in + n, custom_func);
    auto pt1 = std::chrono::high_resolution_clock::now();
    std::cout << "Parallel time = " << 1e-9 * (pt1-pt0).count() << " seconds\n";
      
    std::cout << "Sorted keys:" ;
    for(auto i:keys)
      std::cout << i << " ";
    std::cout << "\n" << "Sorted data:";
    for(auto i:data)
      std::cout << i << " ";
    std::cout << "\n";
  }
    
  return 0;
}
