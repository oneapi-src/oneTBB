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
#include <thread>
#include <tbb/tbb.h>

void printArrival(tbb::task_arena::priority priority);
void waitUntil(int N);

void explicitArenaWithPriority(tbb::task_arena::priority priority) {
  tbb::task_arena a{tbb::info::default_concurrency(), 1, priority};
  a.execute([=]() {
    tbb::parallel_for(0, 
                      10*tbb::info::default_concurrency(), 
                      [=](int) { printArrival(priority); });
  });
}

void runTwoThreads(tbb::task_arena::priority priority0, 
                   tbb::task_arena::priority priority1) {
  std::thread t0([=]() {
    waitUntil(2);
    explicitArenaWithPriority(priority0);
  });
  std::thread t1([=]() {
    waitUntil(2);
    explicitArenaWithPriority(priority1);
  });
  t0.join();
  t1.join();
}

#include <cstdio>

void printArrival(tbb::task_arena::priority priority) {
  switch (priority) {
    case tbb::task_arena::priority::low:
      std::printf(" low ");
      break;
    case tbb::task_arena::priority::normal:
      std::printf(" normal ");
      break;
    case tbb::task_arena::priority::high:
      std::printf(" high ");
      break;
    default:
      break;
  }
  std::fflush(stdout);
  tbb::tick_count t0 = tbb::tick_count::now();
  while ((tbb::tick_count::now() - t0).seconds() < 0.01);
}

std::atomic<int> count_up = 0;
void waitUntil(int N) {
  ++count_up;
  while (count_up != N);
}

int main() {
  count_up = 0;
  std::printf("\n\n\n\nrunTwoThreads(low, high)");
  runTwoThreads(tbb::task_arena::priority::low, tbb::task_arena::priority::high);

  count_up = 0;
  std::printf("\n\n\n\nrunTwoThreads(low, normal)");
  runTwoThreads(tbb::task_arena::priority::low, tbb::task_arena::priority::normal);

  count_up = 0;
  std::printf("\n\n\n\nrunTwoThreads(normal, high)");
  runTwoThreads(tbb::task_arena::priority::normal, tbb::task_arena::priority::high);
  return 0;
}

