//==============================================================
// Copyright (c) 2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// =============================================================

#include <CL/sycl.hpp>

#define N 1024

using namespace sycl;

int main() {
    
    std::vector<int> c_host(N);
    std::vector<int> c_res(N);
    std::fill_n(c_res.begin(), N, 1);
    
    {
        queue q(gpu_selector{});
        
        int *a = malloc_shared<int>(N, q);
        int *b = malloc_shared<int>(N, q);
        int *c_dev = malloc_device<int>(N, q);
        
        for (int i = 0; i < N; ++i) {
            a[i] = i + 1;
            b[i] = -i;
        }
        
        auto e1 = q.parallel_for(range<1>(N), [=](id<1> i) { c_dev[i] = a[i] + b[i]; });
                 
        q.submit([&](auto &h) {
              h.depends_on(e1);
              h.memcpy(c_host.data(), c_dev, sizeof(int) * N);
        }).wait();
                 
        free(a,q);
        free(b,q);
        free(c_dev,q);
    }

    if (std::equal(c_host.begin(), c_host.end(), c_res.begin())) {
        std::cout << "DONE.\n";
    } else {
        std::cout << "ERROR. Mismatch in resulting data!\n";
    }
    
    return 0;
}
