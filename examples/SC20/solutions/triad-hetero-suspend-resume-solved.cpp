//==============================================================
// Copyright (c) 2020 Intel Corporation
//
// SPDX-License-Identifier: Apache 2.0
// =============================================================

#include "../common/common_utils.hpp"

#include <array>
#include <atomic>
#include <cmath>
#include <iostream>
#include <thread>
#include <algorithm>

#include <CL/sycl.hpp>

#include <tbb/blocked_range.h>
#include <tbb/task.h>
#include <tbb/task_group.h>
#include <tbb/parallel_for.h>

template<size_t array_size>
class AsyncActivity {
    float alpha;
    const float *a_array, *b_array; 
    float *c_sycl;
    sycl::queue& q;
    float offload_ratio;
    std::atomic<bool> submit_flag;
    tbb::task::suspend_point suspend_point;
    std::thread service_thread;

public:
    AsyncActivity(float alpha_sycl, const float *a, const float *b, float *c, sycl::queue &queue) : 
      alpha{alpha_sycl}, a_array{a}, b_array{b}, c_sycl{c}, q{queue}, 
      offload_ratio{0}, submit_flag{false},
      service_thread([this] {
        // Wait until the job will be submitted into the async activity
        while(!submit_flag) std::this_thread::yield();
        // Here submit_flag==true --> DISPATCH GPU computation
        std::size_t array_size_sycl = std::ceil(array_size * offload_ratio);
        float l_alpha=alpha;
        const float *la=a_array, *lb=b_array;
        float *lc=c_sycl;
        q.submit([&](sycl::handler& h) {            
            h.parallel_for(sycl::range<1>{array_size_sycl}, [=](sycl::id<1> index) {
              lc[index] = la[index] + lb[index] * l_alpha;
            });  
        }).wait(); //The thread may spin or block here.
  
        // Pass a signal into the main thread that the GPU work is completed
        tbb::task::resume(suspend_point);
      }) {}

    ~AsyncActivity() {
        service_thread.join();
    }

    void submit( float ratio, tbb::task::suspend_point sus_point ) {
        offload_ratio = ratio;
        suspend_point = sus_point;
        submit_flag = true;
    }
}; // class AsyncActivity

int main() {
  
  constexpr float ratio = 0.5; // CPU or GPU offload ratio
  // We use different alpha coefficients so that 
  //we can identify the GPU and CPU part if we print c_array result
  const float alpha_sycl = 0.5, alpha_tbb = 1.0;  
  constexpr size_t array_size = 16;

  sycl::queue q{sycl::gpu_selector{}};
  std::cout << "Using device: " << q.get_device().get_info<sycl::info::device::name>() << '\n'; 

  //This host allocation of c comes handy specially for integrated GPUs (CPU and GPU share mem)
  float *a_array = malloc_host<float>(array_size, q); 
  float *b_array = malloc_host<float>(array_size, q); 
  float *c_sycl = malloc_host<float>(array_size, q);
  float *c_tbb = new float[array_size];

  // sets array values to 0..N
  std::iota(a_array, a_array+array_size,0); 
  std::iota(b_array, b_array+array_size,0);
  
  tbb::task_group tg;
  AsyncActivity<array_size> activity{alpha_sycl, a_array, b_array, c_sycl, q};

  //Spawn a task that runs a parallel_for on the CPU
  tg.run([&, alpha_tbb]{
   std::size_t i_start = static_cast<std::size_t>(std::ceil(array_size * ratio));
   std::size_t i_end = array_size;
   tbb::parallel_for(i_start, i_end, [=]( std::size_t index ) {
     c_tbb[index] = a_array[index] + alpha_tbb * b_array[index];
   });
  });

  //Spawn another task that asyncrhonously offloads computation to the GPU  
    tbb::task::suspend([&]( tbb::task::suspend_point suspend_point ) {
     activity.submit(ratio, suspend_point);
    });

  tg.wait();

  //Merge GPU result into CPU array
  std::size_t gpu_end = static_cast<std::size_t>(std::ceil(array_size * ratio));
  std::copy(c_sycl, c_sycl+gpu_end, c_tbb);

  common::validate_usm_results(ratio, alpha_sycl, alpha_tbb, a_array, b_array, c_tbb, array_size);
  if(array_size<64)
    common::print_usm_results(ratio, alpha_sycl, alpha_tbb, a_array, b_array, c_tbb, array_size);

  free(a_array,q);
  free(b_array,q);
  free(c_sycl,q);
  delete[] c_tbb;
}
