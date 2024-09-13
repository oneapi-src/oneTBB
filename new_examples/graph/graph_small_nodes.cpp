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
#include "tbb/tbb.h"

const int N = 1000;

void small_nodes() {
  tbb::flow::graph g;

  tbb::flow::function_node add( g, tbb::flow::unlimited, 
                     [](const int &v) { return v+1; } );
  tbb::flow::function_node multiply( g, tbb::flow::unlimited, 
                          [](const int &v) { return v*2; } );
  tbb::flow::function_node cube( g, tbb::flow::unlimited, 
                      [](const int &v) { return v*v*v; } );

  tbb::flow::make_edge(add, multiply);
  tbb::flow::make_edge(multiply, cube);

  for(int i = 1; i <= N; ++i)
    add.try_put(i);
  g.wait_for_all();
}

void small_nodes_combined() {
  tbb::flow::graph g;

  tbb::flow::function_node< int, int > combined_node( g, tbb::flow::unlimited, 
                     [](const int &v) { 
                        auto v2 = (v+1)*2;
                        return v2*v2*v2;
                     });

  for(int i = 1; i <= N; ++i)
    combined_node.try_put(i);
  g.wait_for_all();
}

void small_nodes_lightweight() {
  tbb::flow::graph g;

  tbb::flow::function_node< int, int > add( g, tbb::flow::unlimited, 
                     [](const int &v) { return v+1; } );
  tbb::flow::function_node< int, int, tbb::flow::lightweight >  multiply( g, tbb::flow::unlimited, 
                          [](const int &v) { return v*2; } );
  tbb::flow::function_node< int, int, tbb::flow::lightweight >  cube( g, tbb::flow::unlimited, 
                      [](const int &v) { return v*v*v; } );

  tbb::flow::make_edge(add, multiply);
  tbb::flow::make_edge(multiply, cube);

  for(int i = 1; i <= N; ++i)
    add.try_put(i);
  g.wait_for_all();
}

void small_nodes_combined_lightweight() {
  tbb::flow::graph g;

  tbb::flow::function_node< int, int, tbb::flow::lightweight > combined_node( g, tbb::flow::unlimited, 
                     [](const int &v) { 
                        auto v2 = (v+1)*2;
                        return v2*v2*v2;
                     });

  for(int i = 1; i <= N; ++i)
    combined_node.try_put(i);
  g.wait_for_all();
}

static void warmupTBB();

int main() {
  warmupTBB();
  tbb::tick_count t0 = tbb::tick_count::now();
  small_nodes();
  double ts = (tbb::tick_count::now() - t0).seconds();
  t0 = tbb::tick_count::now();
  small_nodes_combined();
  double ts_combined = (tbb::tick_count::now() - t0).seconds();
  t0 = tbb::tick_count::now();
  small_nodes_lightweight();
  double ts_lightweight = (tbb::tick_count::now() - t0).seconds();
  t0 = tbb::tick_count::now();
  small_nodes_combined_lightweight();
  double ts_combined_lightweight = (tbb::tick_count::now() - t0).seconds();
  std::cout << "small nodes combined / small nodes ==  " << ts_combined / ts << "\n";
  std::cout << "small nodes lightweight / small nodes ==  " << ts_lightweight / ts << "\n";
  std::cout << "small nodes combined & lightweight / small nodes ==  " << ts_combined_lightweight / ts << "\n";
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