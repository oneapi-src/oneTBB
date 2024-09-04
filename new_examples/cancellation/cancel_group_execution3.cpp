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

#include <iostream>
#include <vector>
#include <tbb/task_group.h>
#include <tbb/tick_count.h>

int grainsize = 100;
std::vector<int> data;
int myindex=-1;
tbb::task_group g;

void SerialSearch(long begin, long end) {
  for(int i=begin; i<end; ++i){
    if(data[i]==-2){
      myindex=i;
      g.cancel();
      break;
    }
  }
}

void ParallelSearch(long begin, long end) {
  //uncommenting the following line will speedup this program even more!
  //if(g.is_group_execution_cancelled()) return;
  //if(g.context().is_group_execution_cancelled()) return;
  //if(g.is_current_task_group_canceling()) return;
  if(g.is_canceling()) return;
  if((end-begin) < grainsize) { //cutoof equivalent
    return SerialSearch(begin, end);
  }
  else{
    long mid=begin+(end-begin)/2;
    g.run([&]{ParallelSearch(begin, mid);}); // spawn a task
    g.run([&]{ParallelSearch(mid, end);});   // spawn another task
  }
}

int main(int argc, char** argv)
{
  int n = 100000000;
  data.resize(n);
  data[n/2] = -2;

  auto t0 = tbb::tick_count::now();
  SerialSearch(0,n);
  auto t1 = tbb::tick_count::now();
  ParallelSearch(0,n);
  g.wait();     // wait for all spawned tasks
  auto t2 = tbb::tick_count::now();
  double t_s = (t1 - t0).seconds();
  double t_p = (t2 - t1).seconds();

  std::cout << "SerialSearch:   " << myindex << " Time: " << t_s << std::endl;
  std::cout << "ParallelSearch: " << myindex << " Time: " << t_p
            << " Speedup: "       << t_s/t_p << std::endl;
  return 0;
}
