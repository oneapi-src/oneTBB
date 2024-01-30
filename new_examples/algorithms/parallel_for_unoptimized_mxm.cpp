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

// avoid Windows macros
#define NOMINMAX
#include <algorithm>
#include <iostream>
#include <vector>
#include <tbb/tbb.h>

//
// Forward function declarations:
//
template<typename InMat1, typename InMat2, typename OutMat> 
void simpleSerialMatrixProduct(int M, int N, int K, const InMat1& a, const InMat2& b, OutMat& c);
static void warmupTBB();
static bool resultsAreValid(const std::vector<double>&, const std::vector<double>&, double K);

//
// OVERVIEW
//
// This examples demonstrates a simple matrix product implementation.
// This is NOT optimized for memory or vectorization and so is for 
// syntactic demonstration only.
//

template<typename InMat1, typename InMat2, typename OutMat> 
void simpleParallelMatrixProduct(int M, int N, int K, const InMat1& a, const InMat2& b, OutMat& c) {
  tbb::parallel_for( 0, M, [&](int i0) {
    for (int i1 = 0; i1 < N; ++i1) {
      double& c0 = c[i0*N+i1];
      for (int i2 = 0; i2 < K; ++i2) {
        c0 += a[i0*K+i2] * b[i2*N + i1];
      }
    }
  });
}

int main() {
  const int M = 1024;
  const int N = 1024; 
  const int K = 1024;
  std::vector<double> a(M*K, 1.0), b(K*N, 1.0), c0(M*N, 0.0), c1(M*N, 0.0);

  // Perform serial matrix product
  tbb::tick_count t0 = tbb::tick_count::now();
  simpleSerialMatrixProduct(M, N, K, a, b, c0);
  double serial_time = (tbb::tick_count::now() - t0).seconds();

  // Since this is a trivial example, warmup the library
  // to make the comparison meangingful
  warmupTBB();

  tbb::tick_count t1 = tbb::tick_count::now();
  simpleParallelMatrixProduct(M, N, K, a, b, c1);
  double tbb_time = (tbb::tick_count::now() - t1).seconds();

  if (resultsAreValid(c0, c1, K)) {
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

template<typename InMat1, typename InMat2, typename OutMat> 
void simpleSerialMatrixProduct(int M, int N, int K, const InMat1& a, const InMat2& b, OutMat& c) {
  for (int i0 = 0; i0 < M; ++i0) {
    for (int i1 = 0; i1 < N; ++i1) {
      auto& c0 = c[i0*N+i1]; 
      for (int i2 = 0; i2 < K; ++i2) {
        c0 += a[i0*K+i2] * b[i2*N + i1];
      }
    }
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

static bool resultsAreValid(const std::vector<double>& c0, const std::vector<double>& c1, double K) {
  for (std::size_t i = 0; i < c0.size(); ++i) {
    if (c0[i] != c1[i] && c0[i] != K) {
      std::cerr << "Product FAILED" << std::endl;
      return false;
    }
  }
  return true;
}

