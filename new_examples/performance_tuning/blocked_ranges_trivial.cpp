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
#include <vector>

#define TBB_PREVIEW_BLOCKED_RANGE_ND 1
#include <tbb/tbb.h>

double f(double v);

void example1d(int Z, int P, int N, int M, double* a) {
  tbb::parallel_for(tbb::blocked_range<int>{0,Z}, 
    [=](const tbb::blocked_range<int>& r_i) {
      for (int i = r_i.begin(); i < r_i.end(); ++i)
        for (int j = 0; j < P; ++j)
          for (int k = 0; k < N; ++k) 
            for (int l = 0; l < M; ++l) 
              a[i*P*N*M+j*N*M+k*M+l] = f(a[i*P*N*M+j*N*M+k*M+l]);
      
    }
  );
}

void example2d(int Z, int P, int N, int M, double* a) {
  tbb::parallel_for(tbb::blocked_range2d<int,int>{0,Z,0,P}, 
    [=](const tbb::blocked_range2d<int,int>& r_ij) {
      const auto& r_i = r_ij.rows();
      const auto& r_j = r_ij.cols();
      for (int i = r_i.begin(); i < r_i.end(); ++i)
        for (int j = r_j.begin(); j < r_j.end(); ++j)
          for (int k = 0; k < N; ++k) 
            for (int l = 0; l < M; ++l) 
              a[i*P*N*M+j*N*M+k*M+l] = f(a[i*P*N*M+j*N*M+k*M+l]);
    }
  );
}

void example3d(int Z, int P, int N, int M, double* a) {
  tbb::parallel_for(tbb::blocked_range3d<int,int,int>{0,Z,0,P,0,N}, 
    [=](const tbb::blocked_range3d<int,int,int>& r_ijk) {
      const auto& r_i = r_ijk.pages();
      const auto& r_j = r_ijk.rows();
      const auto& r_k = r_ijk.cols();
      for (int i = r_i.begin(); i < r_i.end(); ++i)
        for (int j = r_j.begin(); j < r_j.end(); ++j)
          for (int k = r_k.begin(); k < r_k.end(); ++k) 
            for (int l = 0; l < M; ++l) 
              a[i*P*N*M+j*N*M+k*M+l] = f(a[i*P*N*M+j*N*M+k*M+l]);
    }
  );
}

void exampleNd(int Z, int P, int N, int M, double* a) {
  tbb::parallel_for(tbb::blocked_rangeNd<int,4>{{0,Z},{0,P},{0,N}, {0,M}}, 
    [=](const tbb::blocked_rangeNd<int,4>& r_ijkl) {
      const auto& r_i = r_ijkl.dim(0);
      const auto& r_j = r_ijkl.dim(1);
      const auto& r_k = r_ijkl.dim(2);
      const auto& r_l = r_ijkl.dim(3);
      for (int i = r_i.begin(); i < r_i.end(); ++i)
        for (int j = r_j.begin(); j < r_j.end(); ++j)
          for (int k = r_k.begin(); k < r_k.end(); ++k) 
            for (int l = r_l.begin(); l < r_l.end(); ++l) 
              a[i*P*N*M+j*N*M+k*M+l] = f(a[i*P*N*M+j*N*M+k*M+l]);
    }
  );
}

#include <iostream>

static void warmupTBB();
static bool resultsAreValid(int, const double*, const double*, const double*, const double*, const double*);
static void serialDouble(int, double*);

int main() {
  const int Z = 8; 
  const int P = 16; 
  const int N = 32; 
  const int M = 64; 
  const int Size = Z*P*N*M;

  std::printf("Allocating: {%d,%d,%d,%d} == %d\n",Z,P,N,M,Size);
  double* a0 = new double[Size];
  double* a1 = new double[Size];
  double* a2 = new double[Size];
  double* a3 = new double[Size];
  double* aN = new double[Size];
  for (int i = 0; i < Size; ++i)
    a0[i] = a1[i] = a2[i] = a3[i] = aN[i] = 1.0;

  // Perform serial double
  tbb::tick_count t0 = tbb::tick_count::now();
  serialDouble(Size, a0);
  double serial_time = (tbb::tick_count::now() - t0).seconds();
  std::printf("serial done\n");

  // Since this is a trivial example, warmup the library
  // to make the comparison meangingful
  warmupTBB();

  t0 = tbb::tick_count::now();
  example1d(Z, P, N, M, a1);
  double tbb1d_time = (tbb::tick_count::now() - t0).seconds();
  std::printf("1d done\n");

  t0 = tbb::tick_count::now();
  example2d(Z, P, N, M, a2);
  double tbb2d_time = (tbb::tick_count::now() - t0).seconds();
  std::printf("2d done\n");

  t0 = tbb::tick_count::now();
  example3d(Z, P, N, M, a3);
  double tbb3d_time = (tbb::tick_count::now() - t0).seconds();
  std::printf("3d done\n");
  
  t0 = tbb::tick_count::now();
  exampleNd(Z, P, N, M, aN);
  double tbbNd_time = (tbb::tick_count::now() - t0).seconds();
  std::printf("Nd done\n");

  if (resultsAreValid(Size, a0, a1, a2, a3, aN)) {
    std::cout << "serial_time == " << serial_time << " seconds\n"
              << "tbb1d_time == " << tbb1d_time << " seconds\n"
              << "speedup == " << serial_time/tbb1d_time << "\n"
              << "tbb2d_time == " << tbb2d_time << " seconds\n"
              << "speedup == " << serial_time/tbb2d_time << "\n"
              << "tbb3d_time == " << tbb3d_time << " seconds\n"
              << "speedup == " << serial_time/tbb3d_time << "\n"
              << "tbbNd_time == " << tbbNd_time << " seconds\n"
              << "speedup == " << serial_time/tbbNd_time << "\n";
    return 0;
  } else {
    std::cout << "ERROR: invalid results!\n";
    return 1;
  }
}

//
// Definitions of functions that had forward declarations
//

void serialDouble(int N, double* a) {
  std::printf("Serial of size %d\n", N);
  for (int i = 0; i < N; ++i) {
    a[i] = f(a[i]);
  };
}

static void warmupTBB() {
  // This is a simple loop that should get workers started.
  // oneTBB creates workers lazily on first use of the library
  // so this hides the startup time when looking at trivial
  // examples that do little real work. 
  tbb::parallel_for(0, tbb::info::default_concurrency(), 
    [=](int) {
      tbb::tick_count t0 = tbb::tick_count::now();
      while ((tbb::tick_count::now() - t0).seconds() < 0.01);
    }
  );
}

double f(double v) {
  tbb::tick_count t0 = tbb::tick_count::now();
  while ((tbb::tick_count::now() - t0).seconds() < 0.00001);
  return 2*v;
}

static bool resultsAreValid(int N, 
                            const double* a0, const double* a1, 
                            const double* a2, const double* a3,
                            const double* aN) {
  for (int i = 0; i < N; ++i) {
    if (a0[i] != 2.0 
        || a1[i] != 2.0
        || a2[i] != 2.0
        || a3[i] != 2.0
        || aN[i] != 2.0) {
      std::printf("%d: %f, %f, %f, %f\n", i, a0[i], a1[i], a2[i], a3[i]); 
      std::cerr << "Invalid results" << std::endl;
      return false;
    }
  }
  return true;
}

