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

#include <cstdio>
#include <memory>
#include <tbb/tbb.h>

const int N = 100000;

#define SM

#ifdef SM
std::shared_ptr<double> a[N];
#else
double *a[N];
#endif

int main() {

  for (int j=0; j<150; j++ ) {
#ifdef SM
  tbb::parallel_for( 0, N-1, [&](int i) { a[i] = std::make_shared<double>(1.0 * i); } );
#else
  tbb::parallel_for( 0, N-1, [&](int i) { a[i] = new double(1.0 * i); } );
#endif
  tbb::parallel_for( 0, N-1, [&](int i) { *a[i] = (*a[i]) * 2.1;       } );
  tbb::parallel_for( 0, N-1, [&](int i) { *a[i] = (*a[i]) + 3.3;       } );
  tbb::parallel_for( 0, N-1, [&](int i) { *a[i] = (*a[i]) * 0.9;       } );
#ifdef SM
  tbb::parallel_for( 0, N-1, [&](int i) { a[i] = NULL; } );
#else
  tbb::parallel_for( 0, N-1, [&](int i) { delete a[i]; } );
#endif
  }

  return 0;
}
