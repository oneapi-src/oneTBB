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

// Compile with: g++ -o p par_histo_coarse.cpp -ltbb -std=c++11 -O3
#include <vector>
#include <iostream>
#include <algorithm>
#include <atomic>

#include <tbb/tick_count.h>
#include <tbb/parallel_invoke.h>


alignas(64) std::atomic<uint32_t> v{1};

void fetch_and_triple(std::atomic<uint32_t>& v)
{
  uint32_t old_v;
  uint32_t new_v;
  do {
    old_v=v; //take a snapshot
    new_v=v*3;
  } while (!v.compare_exchange_strong(old_v, new_v));
}

int main(int argc, char** argv)
{
  long int N = 100; //1000000000;

  std::cout<< "N="<< N << "\t";

  tbb::parallel_invoke(
    [&](){for(int i=0; i<N; ++i) fetch_and_triple(v);},
    [&](){for(int i=0; i<N; ++i) fetch_and_triple(v);}
  );
  std::cout << " v=" <<v <<std::endl;

  return 0;
}
