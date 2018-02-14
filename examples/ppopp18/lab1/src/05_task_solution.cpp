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

template<typename T> void nrand(T& a)
{
  static const double scale = 1.0 / RAND_MAX;
  a = static_cast<T>(scale * (2 * std::rand() - RAND_MAX));
}


/// Pure Quicksort (e.g., tail recursion is not avoided).
template<typename I>
struct parallel_qsort: public tbb::task {
  typedef typename std::iterator_traits<I>::value_type value_type;
  I begin, end;

  parallel_qsort(I begin_, I end_): begin(begin_), end(end_) {}

  tbb::task* execute() {
    tbb::task* result = 0;
    if (begin != end) {
      const I pivot = end - 1;
      const I middle = std::partition(begin, pivot, std::bind2nd(std::less<value_type>(), *pivot));
      std::swap(*pivot, *middle);
      this->set_ref_count(3); // incl. this
      this->spawn(*new(this->allocate_child()) parallel_qsort(begin, middle));
      this->spawn(*new(this->allocate_child()) parallel_qsort(middle + 1, end));
      this->wait_for_all();
    }
    return result;
  }
};


template<typename I>
void compare(I begin, I end, I expect, std::size_t& diff_count)
{
  diff_count = 0;
  for (I i = begin; i != end; ++i, ++expect) {
    if (*i != *expect) {
      ++diff_count;
    }
  }
}


int main(int argc, char* argv[])
{
  try {
    using namespace tbb;
    typedef float T;

    std::size_t size = 1 < argc ? std::atol(argv[1]) : 4 * 1024 * 1024;
    std::vector<T> values(size), result(size);
    std::for_each(values.begin(), values.end(), nrand<T>);

    typedef parallel_qsort<std::vector<T>::iterator> qsort_type;

    std::size_t nrepeat = 5;
    double time_own = std::numeric_limits<double>::max();
    for (std::size_t i = 0; i < nrepeat; ++i) {
      std::copy(values.begin(), values.end(), result.begin());
      tick_count start = tick_count::now();
      qsort_type* qsort = new(tbb::task::allocate_root()) qsort_type(result.begin(), result.end());
      tbb::task::spawn_root_and_wait(*qsort);
      tick_count end = tick_count::now();
      time_own = std::min(time_own, (end - start).seconds());
    }

    std::cout << "Time: " << time_own << " s";

    std::size_t diff_count;
    tbb::parallel_sort(values.begin(), values.end());
    compare(result.begin(), result.end(), values.begin(), diff_count);

    std::cout << "  (ok=";
    const double correct = static_cast<double>(size - diff_count) / size;
    std::cout << static_cast<int>(100.0 * correct + 0.5) << "%)\n";

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
