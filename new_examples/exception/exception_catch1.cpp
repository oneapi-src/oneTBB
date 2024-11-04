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

#include <tbb/tbb.h>
#include <vector>
#include <iostream>

int main(){
  std::vector<int> data(1000);
  try{
    tbb::parallel_for(0, 2000, [&] (int i) {data.at(i)++;});
  }
  catch(const std::out_of_range& ex) {
    std::cout << "Out_of_range: " << ex.what() << std::endl;
  }
  return 0;
}
