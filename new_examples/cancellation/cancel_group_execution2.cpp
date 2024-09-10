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

int main(int argc, char** argv) {
  int n = 1000000000;
  size_t nth = 4;
  tbb::global_control global_limit{tbb::global_control::max_allowed_parallelism, nth};

  std::vector<int> data(n);
  data[500] = -2;
  int index = -1;
  tbb::task_group_context tg;
  auto t1 = tbb::tick_count::now();
  tbb::parallel_for(tbb::blocked_range<int>{0, n},
    [&](const tbb::blocked_range<int>& r){
        for(int i=r.begin(); i!=r.end(); ++i){
          if(data[i] == -2) {
            index = i;
            // commenting out the following line, can increase run time
            tg.cancel_group_execution();
            break;
          }
        }
  }, tg);
  auto t2 = tbb::tick_count::now();
  std::cout << "Index "     << index;
  std::cout << " found in " << (t2-t1).seconds() << " seconds!\n";
  return 0;
}
