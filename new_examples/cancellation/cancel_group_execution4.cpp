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

/******************
 NOTE: This example does NOTHING useful.
 It proves the syntax used in the book for Figure 9-6 is valid
 and will compile.
******************/

#include <tbb/tbb.h>
#include <vector>
#include <iostream>

#define N 100

int main(int argc, char** argv) {
  std::vector<int> data(N);
  tbb::flow::graph g;

  // tbb::flow::function_node<float,float> node{g,…,[&](float a){
  tbb::flow::function_node<float,float> node{g,tbb::flow::unlimited,[&](float a){
    tbb::task_group_context TGC_E(tbb::task_group_context::isolated);  
    // nested parallel_for
    // tbb::parallel_for(0, N, 1, [&](…){/*loop body*/}, TGC_E);
    tbb::parallel_for(0, N, 1,[&](int j){ data[j] = j; }, TGC_E);
    return a;
  }};

  return 0;
}
