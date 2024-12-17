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

void* operator new (size_t size, const std::nothrow_t&) noexcept
{
  if (size == 0) size = 1; 
  if (void* ptr = scalable_malloc (size))
    return ptr;
  return NULL;
}

void* operator new[] (size_t size, const std::nothrow_t&) noexcept
{
  return operator new (size, std::nothrow);
}

void operator delete (void* ptr, const std::nothrow_t&) noexcept
{
  if (ptr != 0) scalable_free (ptr);
}

void operator delete[] (void* ptr, const std::nothrow_t&) noexcept
{
  operator delete (ptr, std::nothrow);
}

/****************/

const int N = 1000000;

// don't forget ulimit –s unlimited on Linux, or STACK:10000000 on Windows
// otherwise this may fail to run

int main() {
  double *a[N];

  tbb::parallel_for( 0, N-1, [&](int i) { a[i] = new double; } );
  tbb::parallel_for( 0, N-1, [&](int i) { delete a[i];       } );

  return 0;
}
