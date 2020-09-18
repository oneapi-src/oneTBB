//==============================================================
// Copyright (c) 2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// =============================================================
//
#pragma once

#include <atomic>
#include <chrono>
#include <tbb/tbb.h>

static int run_test() {
  static int trial = 0;
  thread_local int was_counted = -1;

  std::atomic<int> num_participating_threads = 0;
  const int N = 1000000;
  tbb::parallel_for(0, N, [&num_participating_threads](unsigned int) {
    const double time_per_iteration =  1000; // N * time_per_iteration == 1 second
    if (was_counted != trial) {
      was_counted = trial;
      ++num_participating_threads;
    }
    auto t0 = std::chrono::high_resolution_clock::now();
    while ((std::chrono::high_resolution_clock::now() - t0).count() < time_per_iteration);
  });
  ++trial;
  return num_participating_threads;
}

static int run_test_2() {
  static int trial = 0;
  thread_local int was_counted = -1;

  std::atomic<int> num_participating_threads = 0;
  const int N = 1000000;
  tbb::parallel_for(0, N, [&num_participating_threads](unsigned int) {
    const double time_per_iteration =  1000; // N * time_per_iteration == 1 second
    if (was_counted != trial) {
      was_counted = trial;
      ++num_participating_threads;
    }
    auto t0 = std::chrono::high_resolution_clock::now();
    while ((std::chrono::high_resolution_clock::now() - t0).count() < time_per_iteration);
  });
  ++trial;
  return num_participating_threads;
}

