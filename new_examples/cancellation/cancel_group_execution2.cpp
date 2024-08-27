/*
Copyright (C) 2024 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
OR OTHER DEALINGS IN THE SOFTWARE.

SPDX-License-Identifier: MIT
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
