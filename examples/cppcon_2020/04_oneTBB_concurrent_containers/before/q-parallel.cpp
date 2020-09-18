//==============================================================
// Copyright (c) 2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// =============================================================

#include <iostream>
#include <queue>
#include <thread>
#include <tbb/tbb.h>

#define INCORRECT_QUEUE_TYPE std::priority_queue<int>;

// STEP A: Replace std::priority_queue with tbb::concurrent_priority_queue
using queue_type = INCORRECT_QUEUE_TYPE;
        
int consume(queue_type &myPQ) {
  int local_sum = 0;
  int value = -1;
    
  bool consuming = true;
  while(consuming) {
    // STEP B: replace the pattern of empty, top and pop 
    //         with a single call to myPQ.try_pop(value)
    if (!myPQ.empty()) {
      value = myPQ.top();
      myPQ.pop();
      if (value == -1)
        consuming = false;
      else
        local_sum += value;
    }
  }
  return local_sum;
}

int main() {
  int sum (0);
  int item;

  queue_type myPQ;
 
  std::thread producer([&]() {
    for(int i=0; i<10001; i+=1) {
      myPQ.push(i);
    }
    // to signal the end to the two consumers
    myPQ.push(-1);
    myPQ.push(-1);
  });

  int local_sum1 = 0, local_sum2 = 0;
  std::thread consumer1([&]() { local_sum1 = consume(myPQ); });
  std::thread consumer2([&]() { local_sum2 = consume(myPQ); });

  producer.join();
  consumer1.join();
  consumer2.join();
    
  // prints "total: 50005000" (for 0,10001,1)
  std::cout << "total: " << local_sum1 + local_sum2 << '\n';
  return 0;
}
