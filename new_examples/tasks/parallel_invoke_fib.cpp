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
#include <tbb/tbb.h>

int cutoff = 30;

long fib(long n) {
  if(n<2)
    return n;
  else
    return fib(n-1)+fib(n-2);
}

long parallel_fib(long n) {
  if(n<2) {
    return n;
  }
  else {
    long x, y;
    tbb::parallel_invoke([&]{x=parallel_fib(n-1);},
                         [&]{y=parallel_fib(n-2);});
    return x+y;
  }
}

long parallel_fib_cutoff(long n) {
  if(n<cutoff) {
    return fib(n);
  }
  else {
    long x, y;
    tbb::parallel_invoke([&]{x=parallel_fib_cutoff(n-1);},
                         [&]{y=parallel_fib_cutoff(n-2);});
    return x+y;
  }
}

static void warmupTBB();

int main(int argc, char** argv)
{
  int n = 40;
  size_t nth = 4;
  tbb::global_control gc{tbb::global_control::max_allowed_parallelism, nth};

  auto t0 = tbb::tick_count::now();
  long fib_s = fib(n);
  auto t1 = tbb::tick_count::now();
  warmupTBB();
  auto t2 = tbb::tick_count::now();
  long fib_p = parallel_fib(n);
  auto t3 = tbb::tick_count::now();
  long fib_c = parallel_fib_cutoff(n);
  auto t4 = tbb::tick_count::now();
  double t_s = (t1 - t0).seconds();
  double t_p = (t3 - t2).seconds();
  double t_c = (t4 - t3).seconds();

  std::cout << "SerialFib:   " << fib_s << " Time: " << t_s << "\n";
  std::cout << "ParallelFib: " << fib_p << " Time: " << t_p << " Speedup: " << t_s/t_p << "\n";
  std::cout << "ParallelFibCutoff_30: " << fib_c << " Time: " << t_p << " Speedup: " << t_s/t_c << "\n";
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

