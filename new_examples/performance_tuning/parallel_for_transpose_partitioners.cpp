/*
Copyright (C) 2019 Intel Corporation

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

#include <tbb/tbb.h>
#include <iostream>

double serialTranspose(int N, double *a, double *b) {
  tbb::tick_count t0 = tbb::tick_count::now();
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
      b[j*N+i] = a[i*N+j];
    }
  }
  tbb::tick_count t1 = tbb::tick_count::now();

  return (t1-t0).seconds();   
}

void obliviousTranspose(int N, int ib, int ie, int jb, int je, 
                              double *a, double *b, int gs) {
  int ilen = ie-ib;
  int jlen = je-jb;
  if (ilen > gs ||  jlen > gs) {
     if ( ilen > jlen ) {
       int imid = (ib+ie)/2;
       obliviousTranspose(N, ib, imid, jb, je, a, b, gs);
       obliviousTranspose(N, imid, ie, jb, je, a, b, gs);
     } else {
       int jmid = (jb+je)/2;
       obliviousTranspose(N, ib, ie, jb, jmid, a, b, gs);
       obliviousTranspose(N, ib, ie, jmid, je, a, b, gs);
     }
  } else {
    for (int i = ib; i < ie; ++i) {
      for (int j = jb; j < je; ++j) {
        b[j*N+i] = a[i*N+j];
      }
    }
  }
}

double serialObliviousTranspose(int N, double *a, double *b, int gs) {
   tbb::tick_count t0 = tbb::tick_count::now();
   obliviousTranspose(N, 0, N, 0, N, a, b, gs);
   tbb::tick_count t1 = tbb::tick_count::now();
   return (t1-t0).seconds();   
}

template< typename P >
double pforTranspose(int N, double *a, double *b, int gs) {
   tbb::tick_count t0 = tbb::tick_count::now();
   tbb::parallel_for( tbb::blocked_range<int>(0, N, gs),
     [N, a, b](const tbb::blocked_range<int>& r) {
       int ie = r.end();
       for (int i = r.begin(); i < ie; ++i) {
         for (int j = 0; j < N; ++j) {
           b[j*N+i] = a[i*N+j];
         }
       }
     }, P() 
   );
   tbb::tick_count t1 = tbb::tick_count::now();
   return (t1-t0).seconds();   
}

template<typename P>
double pforTranspose2d(int N, double *a, double *b, int gs) {
  tbb::tick_count t0 = tbb::tick_count::now();
  tbb::parallel_for( tbb::blocked_range2d<int,int>{
        0, N, static_cast<size_t>(gs), 0, 
        N, static_cast<size_t>(gs)},
    [N, a, b](const tbb::blocked_range2d<int,int>& r) {
      int ie = r.rows().end();
      int je = r.cols().end();
      for (int i = r.rows().begin(); i < ie; ++i) {
        for (int j = r.cols().begin(); j < je; ++j) {
          b[j*N+i] = a[i*N+j];
        }
      }
    }, P()
  );
  tbb::tick_count t1 = tbb::tick_count::now();
  return (t1-t0).seconds();
}

void setArray(int N, double *a);
void checkTranspose(int N, double *a);

#define CONSTRAIN_TO_ECORES 1

int main() {
  int N = 2<<12; // 8192
  double *a = new double[N*N];
  double *b = new double[N*N];
  setArray(N, a);
  setArray(N, b);

  #if CONSTRAIN_TO_ECORES
  std::vector<tbb::core_type_id> core_types = tbb::info::core_types();
  tbb::task_arena::constraints c;
  c.set_core_type(core_types.front());
  c.set_max_concurrency(tbb::info::default_concurrency(c) - 2);
  tbb::task_arena ecore_arena(c);
  std::cout << "Using arena with " << ecore_arena.max_concurrency() << " slots\n";
  ecore_arena.execute([&]() {
  #endif

  serialTranspose(N, a, b);
  double ts = serialTranspose(N, a, b);
  checkTranspose(N, b);
  std::cout << "Serial Time = " << ts << std::endl;

  std::cout << "Parallel Times:" << std::endl
            << "grainsize, oblivious, 1d auto, 1d simple, 2d auto, 2d simple" << std::endl;
  for (int gs = 1; gs <= N; gs *= 2) {
    setArray(N, a);
    setArray(N, b);
    serialObliviousTranspose(N, a, b, gs);
    double to = serialObliviousTranspose(N, a, b, gs);
    checkTranspose(N, b);

    setArray(N, a);
    setArray(N, b);
    pforTranspose<tbb::auto_partitioner>(N, a, b, gs);
    double t1d_auto = pforTranspose<tbb::auto_partitioner>(N, a, b, gs);

    setArray(N, a);
    setArray(N, b);
    pforTranspose<tbb::simple_partitioner>(N, a, b, gs);
    double t1d_simple = pforTranspose<tbb::simple_partitioner>(N, a, b, gs);

    setArray(N, a);
    setArray(N, b);
    pforTranspose2d<tbb::auto_partitioner>(N, a, b, gs);
    double t2d_auto = pforTranspose2d<tbb::auto_partitioner>(N, a, b, gs);

    setArray(N, a);
    setArray(N, b);
    pforTranspose2d<tbb::simple_partitioner>(N, a, b, gs);
    double t2d_simple = pforTranspose2d<tbb::simple_partitioner>(N, a, b, gs);

    std::cout << gs 
              << ", " << to 
              << ", " << t1d_auto 
              << ", " << t1d_simple 
              << ", " << t2d_auto
              << ", " << t2d_simple << std::endl;
  }

  #if CONSTRAIN_TO_ECORES
  });
  #endif

  return 0;
}

void setArray(int N, double *a) {
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
      a[i*N+j] = i;
    }
  }
}

void checkTranspose(int N, double *a) {
   for (int i = 0; i < N; ++i) {
     for (int j = 0; j < N; ++j) {
       if (a[i*N+j] != j) {
         std::cout << "Transpose failed" << std::endl;
       }
     }
   }
}
