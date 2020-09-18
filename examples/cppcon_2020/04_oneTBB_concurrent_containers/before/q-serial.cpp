//==============================================================
// Copyright (c) 2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// =============================================================

#include <iostream>
#include <queue>

int main() {
  int sum (0);
  int item;

  std::priority_queue<int> myPQ;
 
  for(int i=0; i<10001; i+=1) {
    myPQ.push(i);
  }
 
  while( !myPQ.empty() ) {
    sum += myPQ.top();
    myPQ.pop();
  }

  // prints "total: 50005000" (for 0,10001,1)
  std::cout << "total: " << sum << '\n';
  return 0;
}
