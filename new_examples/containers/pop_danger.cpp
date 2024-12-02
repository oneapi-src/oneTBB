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
#include <queue>
#include <tbb/parallel_invoke.h>

int main() {
  int sum0(0), sum1(0); 

  std::priority_queue<int> myPQ;
 
  for(int i=0; i<10001; i+=1) {
    myPQ.push(i);
  }
  
  tbb::parallel_invoke( 
    [&]() { 
      while( !myPQ.empty() ) {
	sum0 += myPQ.top();
	myPQ.pop();
      }
    }, 
    [&]() { 
      while( !myPQ.empty() ) {
	sum1 += myPQ.top();
	myPQ.pop();
      }
    }); 

  // if correct (which it is not) this
  // would print "total: 50005000" (for 0,10001,1)
  std::cout << "total: "
	    << sum0+sum1 << '\n';
  return 0;
}

//
// Sample outputs:
// % ./pop_danger
// total: 128379594
// % ./pop_danger
// total: 124432912
// % ./pop_danger
// total: 107697942
// % ./pop_danger
// total: 50005000
// % ./pop_danger
// total: 115224669
// % ./pop_danger
// total: 146790135
// % ./pop_danger
// total: 130683763
// % ./pop_danger
// total: 126960607
//
