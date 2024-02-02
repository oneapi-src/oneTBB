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

#include <vector>
#include <tbb/tbb.h>

struct DataItem { int id; double value; };
using QSVector = std::vector<DataItem>;

template<typename Iterator> void serialQuicksort(Iterator b, Iterator e);

template<typename Iterator>
void parallelQuickSort(Iterator b, Iterator e) {
  const int cutoff = 100;

  if (e - b < cutoff) {
    serialQuicksort(b, e);
  } else {
    // do shuffle
    double pivot_value = b->value;
    Iterator i = b, j = e - 1;
    while (i != j) {
      while (i != j && pivot_value < j->value) --j;
      while (i != j && i->value <= pivot_value) ++i;
      std::iter_swap(i, j);
    }
    std::iter_swap(b, i);

    // recursive call
    tbb::parallel_invoke(
      [=]() { parallelQuickSort(b, i); },
      [=]() { parallelQuickSort(i + 1, e); }
    );
  }
}

#include <algorithm>
#include <cfloat>
#include <iostream>
#include <random>

static QSVector makeQSData(int N);
static void warmupTBB();
static bool resultsAreValid(const QSVector&, const QSVector&);

int main() {
  const int N = 1000000;

  // Create a vectors random elements
  QSVector sv, tv;
  sv = tv = makeQSData(N);

  // Sort one of the vectors serially
  tbb::tick_count t0 = tbb::tick_count::now();
  serialQuicksort(sv.begin(), sv.end());
  double serial_time = (tbb::tick_count::now() - t0).seconds();

  // Since this is a trivial example, warmup the library
  // to make the comparison meangingful
  warmupTBB();

  tbb::tick_count t1 = tbb::tick_count::now();
  parallelQuickSort(tv.begin(), tv.end());
  double tbb_time = (tbb::tick_count::now() - t1).seconds();

  if (resultsAreValid(sv, tv)) {
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

static QSVector makeQSData(int N) {
  QSVector v;

  std::default_random_engine g;
  std::uniform_real_distribution<double> d(0.0, 1.0);

  for (int i = 0; i < N; ++i)
    v.push_back(DataItem{i, d(g)});

  return v;
}

template<typename Iterator>
void serialQuicksort(Iterator b, Iterator e) {
  if (b >= e) return;

  // do shuffle
  double pivot_value = b->value;
  Iterator i = b, j = e-1;
  while (i != j) {
    while (i != j && pivot_value < j->value) --j;
    while (i != j && i->value <= pivot_value) ++i;
    std::iter_swap(i, j);
  }
  std::iter_swap(b, i);

  // recursive call
  serialQuicksort(b, i);
  serialQuicksort(i+1, e);
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

static bool checkIsSorted(const QSVector& v) {
  double max_value = std::numeric_limits<double>::min();
  for (auto e : v) {
    if (e.value < max_value) {
      std::cerr << "Sort FAILED" << std::endl;
      return false;
    }
    max_value = e.value;
  }
  return true;
}

static bool resultsAreValid(const QSVector& v1, const QSVector& v2) {
  return checkIsSorted(v1) && checkIsSorted(v2);
}

