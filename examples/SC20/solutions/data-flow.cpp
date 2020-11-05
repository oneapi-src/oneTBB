//==============================================================
// Copyright (c) 2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// =============================================================

#include <CL/sycl.hpp>
#define SIZE 1024

using namespace sycl;

int main() {
    
    std::array<int, SIZE> c;
    // Reference vector for sanity check
    std::array<int, SIZE> c_res;
    std::fill_n(c_res.begin(), SIZE, 1);
    
    {
        range<1> r{SIZE};
        queue q(gpu_selector{});
        buffer<int, 1> a_buf{r};
        buffer<int, 1> b_buf{r};
        buffer<int, 1> c_buf{c.data(), r};
    
    // Create a simple data dependency graph,
    // Following structure:
    // (1) ----> (3) ---> (4)
    // (2) __/   
    
        q.submit([&](auto& h) {
            accessor a(a_buf, h, write_only);
            h.parallel_for(r, [=](auto idx) {
                a[idx] = idx; }); 
        });
        q.submit([&](auto& h) {
            accessor b(b_buf, h, write_only);
            h.parallel_for(r, [=](auto idx) {
                b[idx] = -idx; }); 
        });
        q.submit([&](auto& h) {
            accessor a(a_buf, h, read_only);
            accessor b(b_buf, h, read_only);
            accessor c(c_buf, h, write_only);
            h.parallel_for(r, [=](auto idx) {
                c[idx] = a[idx] + b[idx]; }); 
        });
        q.submit([&](auto& h) {
            accessor c(c_buf, h, read_write);
            h.parallel_for(r, [=](auto idx) {
                c[idx] += 1; }); 
        }).wait();
    }
    
    if (std::equal(c.begin(), c.end(), c_res.begin())) {
        std::cout << "DONE.\n";
    } else {
        std::cout << "ERROR. Mismatch in resulting data!\n";
    }
    
    return 0;
}
