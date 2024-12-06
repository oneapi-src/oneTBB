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

#include <iostream>
#include <utility>
#include <sycl/sycl.hpp>
#include <tbb/tbb.h>

int main() {
  const int N = 1000;
  int num_iterations = 3;

  try {
    sycl::queue sycl_q;

    tbb::parallel_for(0, num_iterations, 
      [&](int id) {
        int *a = sycl::malloc_shared<int>(N, sycl_q);
        tbb::task::suspend([=,&sycl_q](tbb::task::suspend_point tag) {
          auto sycl_event = sycl_q.fill(a, id, N);
          sycl_q.submit([=](sycl::handler& sycl_h) {
            sycl_h.depends_on(sycl_event); // run after sycl_event is done
            sycl_h.host_task([tag]() { 
              tbb::task::resume(tag);
            });
          });
        });
        if (std::any_of(a, a+N, [id](int i) { return i != id; })) {
          std::printf("ERROR: unexpected fill result\n"); 
        } else {
          std::printf("Well-formed fill for id %d\n", id);
        }
      }
    );
  } catch (const sycl::exception&) {
    std::cout << "No CPU SYCL device on this platform\n";
  }
  return 0;
}

