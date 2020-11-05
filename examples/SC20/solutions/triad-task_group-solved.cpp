//==============================================================
// Copyright (c) 2020 Intel Corporation
//
// SPDX-License-Identifier: Apache 2.0
// =============================================================

#include "../common/common_utils.hpp"
#include <array>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include "tbb/task_group.h"

int main() {
  const float alpha = 0.5;  // coeff for triad calculation
  const size_t array_size = 16;

  std::array<float, array_size> a_array, b_array, c_sycl, c_tbb;

  // sets array values to 0..N
  common::init_input_arrays(a_array, b_array); 

  // create task_group
  tbb::task_group tg;

  // Run a TBB task that uses SYCL to offload to GPU, function run does not block
  tg.run([&, alpha]() {
    std::cout << "executing on the GPU using SYCL\n";
    {  
      sycl::buffer a_buffer{a_array}, b_buffer{b_array}, c_buffer{c_sycl};
      sycl::queue q{sycl::default_selector{}};
      q.submit([&](sycl::handler& h) {            
        sycl::accessor a_accessor{a_buffer, h, sycl::read_only};
        sycl::accessor b_accessor{b_buffer, h, sycl::read_only};
        sycl::accessor c_accessor{c_buffer, h, sycl::write_only};
        h.parallel_for(sycl::range<1>{array_size}, [=](sycl::id<1> index) {
           c_accessor[index] = a_accessor[index] + b_accessor[index] * alpha;
        });  
      }).wait();
    }
  });

  // Run a TBB task that uses SYCL to offload to CPU
  tg.run([&, alpha]() {
    std::cout << "executing on the CPU using TBB\n";

    tbb::parallel_for(tbb::blocked_range<int>(0, a_array.size()),
      [&](tbb::blocked_range<int> r) {
        for (int index = r.begin(); index < r.end(); ++index) {
          c_tbb[index] = a_array[index] + b_array[index] * alpha;
        }
    });
  });

  // wait for both TBB tasks to complete
  tg.wait();

  common::validate_results(alpha, a_array, b_array, c_sycl, c_tbb);
  common::print_results(alpha, a_array, b_array, c_sycl, c_tbb);
} 
