/*
    Copyright (c) 2005-2023 Intel Corporation

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

#include <vector>
#include <tbb/tbb.h>

double f(double v);

void example(int N, std::vector<double>& a) {
  tbb::parallel_for(0, N, [&](int i) {
    a[i] = f(a[i]);
  });
}

#include <iostream>

static void warmupTBB();
static bool resultsAreValid(const std::vector<double>&, const std::vector<double>&);
static void serialDouble(int, std::vector<double>&);

int main() {
  const int N = 1024; 
  std::vector<double> a0(N, 1.0);
  std::vector<double> a1(N, 1.0);

  // Perform serial double
  tbb::tick_count t0 = tbb::tick_count::now();
  serialDouble(N, a0);
  double serial_time = (tbb::tick_count::now() - t0).seconds();

  // Since this is a trivial example, warmup the library
  // to make the comparison meangingful
  warmupTBB();

  tbb::tick_count t1 = tbb::tick_count::now();
  example(N, a1);
  double tbb_time = (tbb::tick_count::now() - t1).seconds();

  if (resultsAreValid(a0, a1)) {
    std::cout << "serial_time == " << serial_time << " seconds\n"
              << "tbb_time == " << tbb_time << " seconds\n"
              << "speedup == " << serial_time/tbb_time << "\n";
    return 0;
  } else {
    std::cout << "ERROR: invalid results!\n";
    return 1;
  }
}

//
// Definitions of functions that had forward declarations
//

void serialDouble(int N, std::vector<double>& a) {
  for (int i = 0; i < N; ++i) {
    a[i] = f(a[i]);
  };
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

double f(double v) {
  tbb::tick_count t0 = tbb::tick_count::now();
  while ((tbb::tick_count::now() - t0).seconds() < 0.01);
  return 2*v;
}

static bool resultsAreValid(const std::vector<double>& a0, const std::vector<double>& a1) {
  for (std::size_t i = 0; i < a0.size(); ++i) {
    if (a0[i] != a1[i] && a0[i] != 2.0) {
      std::cerr << "Invalid results" << std::endl;
      return false;
    }
  }
  return true;
}

