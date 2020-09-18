//==============================================================
// Copyright (c) 2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// =============================================================

#include <exception>
#include <iostream>
#include <tbb/tbb.h>

#define INCORRECT_VALUE -1
thread_local int my_tid = INCORRECT_VALUE;

class tid_observer : public tbb::task_scheduler_observer {
public:
  tid_observer(tbb::task_arena &a) : tbb::task_scheduler_observer{a} { observe(true); }
  void on_scheduler_entry( bool is_worker ) override {
    // STEP 3.A: replace the INCORRECT_VALUE with a call to tbb::this_task_arena::current_thread_index()
    my_tid = INCORRECT_VALUE;
  }
};

int main() {
  tbb::task_arena a;
  // STEP 3.B: construct a tid_observer, passing "a" to the constructor
  try {
    a.execute([]() {
      tbb::parallel_for(0, 1000000, [](int i) {
        const int time_per_iteration = 1000; // 1 us
        auto t0 = std::chrono::high_resolution_clock::now();
        while ((std::chrono::high_resolution_clock::now() - t0).count() < time_per_iteration);
        if (my_tid == -1 || my_tid != tbb::this_task_arena::current_thread_index()) {
          std::cout << "my_tid not set correctly on entry!\n";
          throw std::exception{};
        }
      });
    });
  } catch (...) {
    std::cout << "Did not complete loop.\n";
    return 1;
  }
  std::cout << "Completed loop.\n";
  return 0;
}
