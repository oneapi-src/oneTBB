/*
    Copyright (c) 2024 Intel Corporation

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <iostream>
#include <string>
#include <tbb/tbb.h>

static inline void spinWaitForAtLeast(double sec=0.0);

static inline double executeFor(int num_trials, int N, double tpi) {
  tbb::tick_count t0;
  for (int t = -1; t < num_trials; ++t) {
    if (!t) t0 = tbb::tick_count::now();
    for (int i = 0; i < N; ++i) {
      spinWaitForAtLeast(tpi);
    } 
  }
  tbb::tick_count t1 = tbb::tick_count::now();
  return (t1 - t0).seconds()/num_trials;
}

template< typename P >
static inline double executePfor(int num_trials, int N,
		                         int gs, P& p, double tpi) {
  tbb::tick_count t0;
  for (int t = -1; t < num_trials; ++t) {
    if (!t) t0 = tbb::tick_count::now();
    tbb::parallel_for (
      tbb::blocked_range<int>{0, N, static_cast<size_t>(gs)},
      [tpi](const tbb::blocked_range<int>& r) {
        for (int i = r.begin(); i < r.end(); ++i) {
          spinWaitForAtLeast(tpi);
        } 
      }, 
      p
    );
  }
  tbb::tick_count t1 = tbb::tick_count::now();
  return (t1 - t0).seconds()/num_trials;
}

int main() {
  tbb::auto_partitioner auto_p;
  tbb::simple_partitioner simple_p;
  tbb::static_partitioner static_p;
  const std::string pname[4] = {"simple", "auto", "affinity", "static"};

  const int N = 262144;
  const int T = 20;
  const double ten_ns = 0.00000001;
  const double twenty_us = 0.00002;
  double timing[4][19];

  for (double tpi = ten_ns; tpi < twenty_us; tpi *= 10) { 
    std::cout << "Speedups for " << tpi << " seconds per iteration" << std::endl
              << "partitioner";
    for (int gs = 1, i = 0; gs <= N; gs *= 2, ++i) 
      std::cout << ", " << gs;
    std::cout << std::endl;

    double serial_time = executeFor(T, N, tpi);

    for (int gs = 1, i = 0; gs <= N; gs *= 2, ++i) {
      tbb::affinity_partitioner affinity_p;
      spinWaitForAtLeast(0.001);
      timing[0][i] = executePfor(T, N, gs, simple_p, tpi);
      timing[1][i] = executePfor(T, N, gs, auto_p, tpi);
      timing[2][i] = executePfor(T, N, gs, affinity_p, tpi);
      timing[3][i] = executePfor(T, N, gs, static_p, tpi);
    }
    for (int p = 0; p < 4; ++p) {
      std::cout << pname[p];  
      for (int gs = 1, i = 0; gs <= N; gs *= 2, ++i) 
        std::cout << ", " << serial_time/timing[p][i];
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }

  return 0;
}

static inline void spinWaitForAtLeast(double sec) {
  if (sec == 0.0) return;
  tbb::tick_count t0 = tbb::tick_count::now();
  while ((tbb::tick_count::now() - t0).seconds() < sec);
}
