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

#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_priority_queue.h>
#include <iostream>

int myarray[10] = { 16, 64, 32, 512, 1, 2, 512, 8, 4, 128 };

void pval(int test, int val) {
  if (test) {
    std::cout << " " << val;
  } else {
    std::cout << " ***";
  }
}

void simpleQ() {
  tbb::concurrent_queue<int> queue;
  int val = 0;

  for( int i=0; i<10; ++i )
    queue.push(myarray[i]);

  std::cout << "Simple  Q   pops are";

  for( int i=0; i<10; ++i )
    pval( queue.try_pop(val), val );

  std::cout << std::endl;
}

void prioQ() {
  tbb::concurrent_priority_queue<int> queue;
  int val = 0;

  for( int i=0; i<10; ++i )
    queue.push(myarray[i]);

  std::cout << "Prio    Q   pops are";

  for( int i=0; i<10; ++i )
    pval( queue.try_pop(val), val );

  std::cout << std::endl;
}

void prioQgt() {
  tbb::concurrent_priority_queue<int,std::greater<int>> queue;
  int val = 0;

  for( int i=0; i<10; ++i )
    queue.push(myarray[i]);

  std::cout << "Prio    Qgt pops are";

  for( int i=0; i<10; ++i )
    pval( queue.try_pop(val), val );

  std::cout << std::endl;
}

void boundedQ() {
  tbb::concurrent_bounded_queue<int> queue;
  int val = 0;

  queue.set_capacity(6);

  for( int i=0; i<10; ++i )
    queue.try_push(myarray[i]);

  std::cout << "Bounded Q   pops are";

  for( int i=0; i<10; ++i )
    pval( queue.try_pop(val), val );

  std::cout << std::endl;
}

int main() {
  simpleQ();
  boundedQ();
  prioQ();
  prioQgt();
  return 0;
}





