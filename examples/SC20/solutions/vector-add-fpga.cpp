//==============================================================
// Copyright (c) 2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// =============================================================

#include <CL/sycl.hpp>
#include <CL/sycl/INTEL/fpga_extensions.hpp>

#define SIZE 1024

using namespace sycl;

int main() {
    
    {
        range<1> r{SIZE};
        #ifdef FPGA_EMULATOR
        INTEL::fpga_emulator_selector device_selector;
        #else
        INTEL::fpga_selector device_selector;
        #endif
        queue q(device_selector);
        
        buffer<int, 1> a_buf{r};
        buffer<int, 1> b_buf{r};
        buffer<int, 1> c_buf{r};
    
    // a ---- c --- d
    // b __/   
    
        q.submit([&](handler& h) {
            accessor a(a_buf, h, write_only);
            h.parallel_for(r, [=](auto idx) {
                a[idx] = idx; }); 
        });
        q.submit([&](handler& h) {
            accessor b(b_buf, h, write_only);
            h.parallel_for(r, [=](auto idx) {
                b[idx] = -idx; }); 
        });
        q.submit([&](handler& h) {
            accessor a(a_buf, h, read_only);
            accessor b(b_buf, h, read_only);
            accessor c(c_buf, h, write_only);
            h.parallel_for(r, [=](auto idx) {
                c[idx] = a[idx] + b[idx]; }); 
        });
        q.submit([&](handler& h) {
            accessor c(c_buf, h, read_write);
            h.parallel_for(r, [=](auto idx) {
                c[idx] += 1; }); 
        }).wait();
    }
    std::cout << "DONE.\n";
    
    return 0;
}
