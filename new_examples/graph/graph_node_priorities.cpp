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

#include <atomic>
#include "tbb/tbb.h"

static void wait_for_ms(int ms);

void inAnyOrder() {
  tbb::task_arena a(2);
  std::printf("No priorities:\n");
  a.execute([]() {
    tbb::flow::graph g;

    tbb::flow::broadcast_node<int> b{g};

    tbb::flow::function_node
    n1{g, tbb::flow::unlimited, 
    [](const int& t) { std::printf("Hi from n1\n"); return 0; }};

    tbb::flow::function_node
    n2{g, tbb::flow::unlimited, 
    [](const int& t) { std::printf("Hi from n2\n"); return 0; }};

    tbb::flow::function_node
    n3{g, tbb::flow::unlimited, 
       [](const int& t) { std::printf("n3: Me first!\n"); return 0; }};

    tbb::flow::make_edge(b, n1);
    tbb::flow::make_edge(b, n2);
    tbb::flow::make_edge(b, n3);

    for (int i = 0; i < 5; ++i) b.try_put(10);
    g.wait_for_all();
  });
}

void withPriorities() {
  tbb::task_arena a(2);
  std::printf("With priorities:\n");
  a.execute([]() {
    tbb::flow::graph g;

    tbb::flow::broadcast_node<int> b{g};

    tbb::flow::function_node
    n1{g, tbb::flow::unlimited, 
    [](const int& t) { std::printf("Hi from n1\n"); return 0; }};

    tbb::flow::function_node
    n2{g, tbb::flow::unlimited, 
    [](const int& t) { std::printf("Hi from n2\n"); return 0; }};

    tbb::flow::function_node
    n3{g, tbb::flow::unlimited, 
       [](const int& t) { std::printf("n3: Me first!\n"); return 0; },
       tbb::flow::node_priority_t(1)};

    tbb::flow::make_edge(b, n1);
    tbb::flow::make_edge(b, n2);
    tbb::flow::make_edge(b, n3);

    for (int i = 0; i < 5; ++i) b.try_put(10);
    g.wait_for_all();
  });
}

#include <iostream>
#include <chrono>
#include <thread>

static void warmupTBB();

int main() {
  warmupTBB();

  tbb::tick_count t0 = tbb::tick_count::now();
  inAnyOrder();
  double t_any = (tbb::tick_count::now() - t0).seconds();

  t0 = tbb::tick_count::now();
  withPriorities();
  double t_prio = (tbb::tick_count::now() - t0).seconds();

  std::cout << "inAnyOrder    : " << t_any << " seconds\n";
  std::cout << "withPriorities: " << t_prio << " seconds\n";
  return 0;
}

static void wait_for_ms(int ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
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