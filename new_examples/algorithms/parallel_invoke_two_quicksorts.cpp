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

// avoid Windows macros
#define NOMINMAX
#include <iostream>
#include <vector>
#include <limits>
#include <tbb/tbb.h>
#include <random>
#include <cfloat>

//
// The data type used in this example
//
struct DataItem { int id; double value; };
using QSVector = std::vector<DataItem>;

//
// Forward function declarations:
//
static QSVector makeQSData(int N);
template<typename Iterator> void serialQuicksort(Iterator b, Iterator e);
static void warmupTBB();
static bool resultsAreValid(const QSVector&, const QSVector&, 
                            const QSVector&, const QSVector&);

//
// OVERVIEW
//
// This examples demonstrates how parallel_invoke can be used to execute 
// functions calls in parallel. The function used to demonstrate this 
// implements quicksort, and is compared against two sequential calls to
// quicksort.
//

int main(int argc, char *argv[]) {
  const int N = 10000000;

  // Create 4 vectors with the same random elements
  QSVector sv1, sv2, tv1, tv2;
  tv2 = tv1 = sv2 = sv1 = makeQSData(N);

  // Sort two of the vectors serially, one after the other
  tbb::tick_count t0 = tbb::tick_count::now();
  serialQuicksort(sv1.begin(), sv1.end());
  serialQuicksort(sv2.begin(), sv2.end());
  double serial_time = (tbb::tick_count::now() - t0).seconds();

  // Since this is a trivial example, warmup the library
  // to make the comparison meangingful
  warmupTBB();

  // Sort the other two vectors in parallel using parallel_invoke
  tbb::tick_count t1 = tbb::tick_count::now();
  tbb::parallel_invoke(
    [&tv1]() { serialQuicksort(tv1.begin(), tv1.end()); },
    [&tv2]() { serialQuicksort(tv2.begin(), tv2.end()); }
  );
  double tbb_time = (tbb::tick_count::now() - t1).seconds();

  if (resultsAreValid(sv1, sv2, tv1, tv2)) {
    std::cout << "serial_time == " << serial_time << " seconds" << std::endl
              << "tbb_time == " << tbb_time << " seconds" << std::endl
              << "speedup == " << serial_time/tbb_time << std::endl
              << "NOTE: the maximum possible speedup is 2" << std::endl;
  } 
  return 0;
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
  QSVector::iterator i = b, j = e-1;
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

static bool resultsAreValid(const QSVector& v1, const QSVector& v2, 
                             const QSVector& v3, const QSVector& v4) {
  return checkIsSorted(v1) && checkIsSorted(v2) 
         && checkIsSorted(v3) && checkIsSorted(v4);
}


