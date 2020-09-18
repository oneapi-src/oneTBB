//==============================================================
// Copyright (c) 2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// =============================================================

#include <iostream>
#include "mkl.h"

#include <tbb/tbb.h>

int main() {
    const int N = 5;
    
    double *A, *B, *C0, *C1, *C2, *C3;
    int m, n, k, i, j;
    double alpha, beta;
    
    m = 2000, k = 200, n = 1000;
    
    A = (double *)mkl_malloc( m*k*sizeof( double ), 64 );
    B = (double *)mkl_malloc( k*n*sizeof( double ), 64 );
    C0 = (double *)mkl_malloc( m*n*sizeof( double ), 64 );
    C1 = (double *)mkl_malloc( m*n*sizeof( double ), 64 );
    C2 = (double *)mkl_malloc( m*n*sizeof( double ), 64 );
    C3 = (double *)mkl_malloc( m*n*sizeof( double ), 64 );

    alpha = 1.0; beta = 0.0;    

    for (i = 0; i < (m*k); i++) {
        A[i] = (double)(i+1);
    }

    for (i = 0; i < (k*n); i++) {
        B[i] = (double)(-i-1);
    }

    for (i = 0; i < (m*n); i++) {
        C0[i] = C1[i] = C2[i] = C3[i] = 0.0;
    }
    
    std::cout << "\nRunning DGEMM in serial:\n";
    auto st0 = std::chrono::high_resolution_clock::now();
    
    for(int i = 0; i <= N; ++i) {
    
        if(i==1) st0 = std::chrono::high_resolution_clock::now();
        cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, 
                m, n, k, alpha, A, k, B, n, beta, C0, n);
        cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, 
                m, n, k, alpha, A, k, B, n, beta, C1, n);
        cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, 
                m, n, k, alpha, A, k, B, n, beta, C2, n);
        cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, 
                m, n, k, alpha, A, k, B, n, beta, C3, n);       
    }
    
    auto st1 = std::chrono::high_resolution_clock::now();
    std::cout << "Average execution time = " << 1e-6 * (st1-st0).count() / N << " milliseconds\n";
    
    std::cout << "\nRunning DGEMM in parallel:\n";
    auto pt0 = std::chrono::high_resolution_clock::now();
    
    for(int i = 0; i <= N; ++i) {
    
        if(i==1) pt0 = std::chrono::high_resolution_clock::now(); 
        tbb::parallel_invoke(
        [&](){ cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, 
                m, n, k, alpha, A, k, B, n, beta, C0, n); },
        [&](){ cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, 
                m, n, k, alpha, A, k, B, n, beta, C1, n); },
        [&](){ cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, 
                m, n, k, alpha, A, k, B, n, beta, C2, n); },
        [&](){ cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, 
                m, n, k, alpha, A, k, B, n, beta, C3, n); }   
        );      
    }
    
    auto pt1 = std::chrono::high_resolution_clock::now();
    std::cout << "Average execution time = " << 1e-6 * (pt1-pt0).count() / N << " milliseconds\n";
    
    mkl_free(A);
    mkl_free(B);
    mkl_free(C0);
    mkl_free(C1);
    mkl_free(C2);
    mkl_free(C3);

    return 0;
}
