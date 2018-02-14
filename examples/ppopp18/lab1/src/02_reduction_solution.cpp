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

#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <vector>

#include <tbb/tbb.h>

template<typename T>
struct tbb_sum_reduce {
  T result;

  explicit tbb_sum_reduce(const T& init): result(init) {}
  tbb_sum_reduce(const tbb_sum_reduce& /*other*/, tbb::split /*dummy*/): result(0) {}

  template<typename I>
  void operator()(const tbb::blocked_range<I>& r) {
    const I end = r.end();
    for (I i = r.begin(); i != end; ++i) {
      result += *i;
    }
  }

  void join(const tbb_sum_reduce& other) {
    result += other.result;
  }
};


template<typename I>
typename std::iterator_traits<I>::value_type sum_reduce(I begin, I end)
{
  typedef typename std::iterator_traits<I>::value_type T;
  tbb_sum_reduce<T> sum(0);
  tbb::parallel_deterministic_reduce(tbb::blocked_range<I>(begin, end, 8192), sum);
//  tbb::parallel_deterministic_reduce(tbb::blocked_range<I>(begin, end), sum);
  return sum.result;
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

    std::size_t size = 1 < argc ? std::atol(argv[1]) : 4 * 1024 * 1024;
    std::size_t repeat = 2 < argc ? std::atol(argv[2]) : 5;

    std::vector<T> values(size);
    std::for_each(values.begin(), values.end(), nrand<T>);

    T result_tbb = 0;
    double time_tbb = std::numeric_limits<double>::max();
    for (std::size_t i = 0; i < repeat; ++i) {
      const tick_count start = tick_count::now();
      result_tbb = sum_reduce(&values[0], &values[0] + values.size());
      time_tbb = std::min(time_tbb, (tick_count::now() - start).seconds());
    }

    std::cout << "TBB: " << result_tbb << " (" << time_tbb << " s)\n";

    T result_seq = 0;
    double time_seq = std::numeric_limits<double>::max();
    std::vector<T>::iterator end = values.end();
    for (std::size_t i = 0; i < repeat; ++i) {
      result_seq = 0;
      const tick_count start = tick_count::now();
      for (std::vector<T>::iterator i = values.begin(); i != end; ++i) {
        result_seq += *i;
      }
      time_seq = std::min(time_seq, (tick_count::now() - start).seconds());
    }

    std::cout << "SEQ: " << result_seq << " (" << time_seq << " s)\n";

    if (0 != time_tbb) {
      std::cout << "      speedup: " << (time_seq / time_tbb) << "x" << '\n';
    }

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
