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
#include <iostream>

int main() {
  tbb::concurrent_queue<int> queue;
  for( int i=0; i<10; ++i )
    queue.push(i);
  for( tbb::concurrent_queue<int>::const_iterator
       i(queue.unsafe_begin()); 
       i!=queue.unsafe_end();
       ++i )
    std::cout << *i << " ";
  std::cout << std::endl;
  return 0;
}
