//==============================================================
// Copyright (c) 2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// =============================================================
//

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <fstream>
#include <utility>
#include <thread>
#include <typeinfo>

#include <tbb/tbb.h>


static void write_csv(const std::string& partitioner_name,
                      int num_threads, 
                      int values_per_thread, 
                      double execution_time, 
                      int num_body_tasks, 
                      int *v) {
  int num_values = num_threads * values_per_thread;
  std::ofstream f("partitioner.csv");
  
  f << partitioner_name 
    << "," << num_threads 
    << "," << values_per_thread 
    << "," << execution_time 
    << "," << num_body_tasks << "\n";
  f << v[0];
  for (int count = 1; count < num_values; ++count) {
    if (count%values_per_thread == 0) f << "\n";
    else f << ",";
    f << v[count];
  }
}

std::atomic<int> num_body_executions = 0;

template<typename Partitioner>
static auto run_test(const std::string &partitioner_name, int gs = 1) {
  int num_threads = std::thread::hardware_concurrency();
  const int values_per_thread = 1000;
  int num_values = num_threads * values_per_thread;
  int *v = new int[num_values];
  num_body_executions = 0;
  // warmup
  tbb::parallel_for(tbb::blocked_range<int>(0, num_values, gs), 
  [v](const tbb::blocked_range<int> &b) {
      const int time_per_iteration = 1000; // 1 us
      for (int i = b.begin(); i < b.end(); ++i) {
        auto t0 = std::chrono::high_resolution_clock::now();
        while ((std::chrono::high_resolution_clock::now() - t0).count() < time_per_iteration);
        v[i] = 0;
      }
    }, Partitioner{}
  );

  auto t0 = std::chrono::high_resolution_clock::now();
  tbb::parallel_for(tbb::blocked_range<int>(0, num_values, gs),
    [v](const tbb::blocked_range<int> &b) {
      const int time_per_iteration = 100; // 1ns
      ++num_body_executions;
      for (int i = b.begin(); i < b.end(); ++i) {
        auto t0 = std::chrono::high_resolution_clock::now();
        while ((std::chrono::high_resolution_clock::now() - t0).count() < time_per_iteration);
        v[i] = tbb::this_task_arena::current_thread_index();
      }
    }, Partitioner{}
  );
  double execution_time = 1e-9*(std::chrono::high_resolution_clock::now() - t0).count();
  int num_body_tasks = num_body_executions.load();
  write_csv(partitioner_name, num_threads, values_per_thread, execution_time, num_body_tasks, v);
  delete [] v;
  return std::make_pair(execution_time, num_body_tasks);
}

