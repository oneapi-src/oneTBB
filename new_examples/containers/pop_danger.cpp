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
  std::cout << "total: "
	    << sum << '\n';

  return 0;
}
