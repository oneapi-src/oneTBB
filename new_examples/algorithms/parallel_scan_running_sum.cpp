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
#include <vector>
#include <tbb/tbb.h>

int simpleSerialRunningSum(const std::vector<int>& v, std::vector<int>& rsum) {
  int N = v.size();
  rsum[0] = v[0];
  for (int i = 1; i < N; ++i) {
    rsum[i] = rsum[i-1] + v[i];
  }
  int final_sum = rsum[N-1];
  return final_sum;
}

int simpleParallelRunningSum(const std::vector<int>& v, std::vector<int>& rsum) {
  int N = v.size();
  rsum[0] = v[0];
  int final_sum = tbb::parallel_scan(
    /* range = */ tbb::blocked_range<int>(1, N), 
    /* identity = */ (int)0,
    /* scan body */ 
    [&v, &rsum](const tbb::blocked_range<int>& r, 
                int sum, bool is_final_scan) -> int {
      for (int i = r.begin(); i < r.end(); ++i) {
        sum += v[i];
        if (is_final_scan) 
          rsum[i] = sum;
      }
      return sum;
    },
    /* combine body */
    [](int x, int y) {
      return x + y;
    }
  );
  return final_sum;
}

static void warmupTBB();

int main() {
  const int N = 1e4;
  std::vector<int> v(N, 0);
  std::vector<int> serial_rsum(N, 0);
  std::vector<int> tbb_rsum(N, 0);
  for (int i = 0; i < N; ++i) {
    v[i] = i;
  }

  double serial_time = 0.0, tbb_time = 0.0;

  tbb::tick_count t0 = tbb::tick_count::now();
  int serial_sum = simpleSerialRunningSum(v, serial_rsum);
  serial_time = (tbb::tick_count::now() - t0).seconds();
  if (serial_sum != N*(N-1)/2) {
    std::cerr << "ERROR: serial sum is wrong! " 
              << serial_sum << " != " << N*(N-1)/2 
              << std::endl;
  }

  warmupTBB();
  tbb::tick_count t1 = tbb::tick_count::now();
  int tbb_sum = simpleParallelRunningSum(v, tbb_rsum);
  tbb_time = (tbb::tick_count::now() - t1).seconds();
  if (tbb_sum != N*(N-1)/2) {
    std::cerr << "ERROR: tbb sum is wrong! " 
              << tbb_sum << " != " << N*(N-1)/2 
              << std::endl;
  }
 
  if (serial_rsum != tbb_rsum) {
    std::cerr << "ERROR: rsum vectors do not match!" << std::endl;
  }

  std::cout << "serial_time == " << serial_time << " seconds" << std::endl
            << "tbb_time == " << tbb_time << " seconds" << std::endl
            << "speedup == " << serial_time/tbb_time << std::endl;

  return 0;
}

static void warmupTBB() {
  // This is a simple loop that should get workers started.
  // oneTBB creates workers lazily on first use of the library
  // so this hides the startup time when looking at trivial
  // examples that do little real work. 
  tbb::parallel_for(0, tbb::info::default_concurrency(), 
    [=](int) {
      tbb::tick_count t0 = tbb::tick_count::now();
      while ((tbb::tick_count::now() - t0).seconds() < 0.01);
    }
  );
}

