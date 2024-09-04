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
