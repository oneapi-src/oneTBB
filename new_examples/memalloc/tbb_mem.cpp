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

#include <stdio.h>
#include <tbb/tbb.h>
#include <tbb/scalable_allocator.h>

#include <stdio.h>
#include "tbb/tbb.h"

using namespace tbb;

const int N = 1000000;

// don't forget ulimit –s unlimited on Linux, or STACK:10000000 on Windows
// otherwise this will fail to run

int main() {
  double *a[N];

  parallel_for( 0, N-1, [&](int i) { a[i] = new double; } );
  parallel_for( 0, N-1, [&](int i) { delete a[i];       } );

  return 0;
}
