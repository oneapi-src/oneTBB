/*
  Copyright (c) 2018 Intel Corporation

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
#include <cstdlib>
#include <vector>

#include <tbb/tbb.h>


template<typename T>
void gemm(T* result, const T* a, const T* b, std::size_t arows, std::size_t acols, std::size_t bcols)
{
  for (std::size_t i = 0; i < arows; ++i) {
    for (std::size_t j = 0; j < bcols; ++j) {
      result[i*bcols+j] = 0;
    }

    for (std::size_t k = 0; k < acols; ++k) {
      for (std::size_t j = 0; j < bcols; ++j) {
        result[i*bcols+j] += a[i*acols+k] * b[k*bcols+j];
      }
    }
  }
}


template<typename T> void nrand(T& a)
{
  static const double scale = 1.0 / RAND_MAX;
  a = static_cast<T>(scale * (2 * std::rand() - RAND_MAX));
}


int main(int argc, char* argv[])
{
  try {
    using namespace tbb;
    typedef float T;

    const std::size_t arows = 1 < argc ? std::atol(argv[1]) : 512;
    const std::size_t acols = 2 < argc ? std::atol(argv[2]) : arows;
    const std::size_t bcols = 3 < argc ? std::atol(argv[3]) : arows;
    std::vector<T> r(arows * bcols), a(arows * acols), b(acols * bcols);
    std::for_each(a.begin(), a.end(), nrand<T>);
    std::for_each(b.begin(), b.end(), nrand<T>);

    tick_count start = tick_count::now();
    gemm(&r[0], &a[0], &b[0], arows, acols, bcols);
    tick_count end = tick_count::now();

    std::cout << "Time: " << 1000.0 * (end - start).seconds() << " ms\n";

    return EXIT_SUCCESS;
  }
  catch(const std::exception& e) {
    std::cerr << "Error: " << e.what() << '\n';
    return EXIT_FAILURE;
  }
  catch(...) {
    std::cerr << "Error: unknown exception caught!\n";
    return EXIT_FAILURE;
  }
}
