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

#include <tbb/tbb.h>
#include <iostream>

template<typename P>
double obliviousTranspose2(int N, double *a, double *b, int gs) {
  tbb::tick_count t0 = tbb::tick_count::now();
  tbb::parallel_for( tbb::blocked_range2d<int,int>{0, N, static_cast<size_t>(gs), 0, N, static_cast<size_t>(gs)},
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

void setArray(int N, double *a) {
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
      a[i*N+j] = i;
    }
  }
}

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

double obliviousTranspose(int N, double *a, double *b, int gs) {
   tbb::tick_count t0 = tbb::tick_count::now();
   obliviousTranspose(N, 0, N, 0, N, a, b, gs);
   tbb::tick_count t1 = tbb::tick_count::now();
   return (t1-t0).seconds();   
}

template< typename P >
double pfor_1d(int N, double *a, double *b, int gs) {
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

void checkTranspose(int N, double *a) {
   for (int i = 0; i < N; ++i) {
     for (int j = 0; j < N; ++j) {
       if (a[i*N+j] != j) {
         std::cout << "Transpose failed" << std::endl;
       }
     }
   }
}

int main() {
  int N = 2<<13;
  double *a = new double[N*N];
  double *b = new double[N*N];
  setArray(N, a);
  setArray(N, b);

  serialTranspose(N, a, b);
  double ts = serialTranspose(N, a, b);
  checkTranspose(N, b);
  std::cout << "Serial Time = " << ts << std::endl;


  std::cout << "Parallel Times:" << std::endl
            << "grainsize, oblivious, 1d auto, 1d simple, 2d auto, 2d simple" << std::endl;
  for (int gs = 1; gs < 2048; gs *= 2) {
    setArray(N, a);
    setArray(N, b);
    obliviousTranspose(N, a, b, gs);
    double to = obliviousTranspose(N, a, b, gs);
    checkTranspose(N, b);

    setArray(N, a);
    setArray(N, b);
    pfor_1d<tbb::auto_partitioner>(N, a, b, gs);
    double t1d_auto = pfor_1d<tbb::auto_partitioner>(N, a, b, gs);

    setArray(N, a);
    setArray(N, b);
    pfor_1d<tbb::simple_partitioner>(N, a, b, gs);
    double t1d_simple = pfor_1d<tbb::simple_partitioner>(N, a, b, gs);

    setArray(N, a);
    setArray(N, b);
    obliviousTranspose2<tbb::auto_partitioner>(N, a, b, gs);
    double t2d_auto = obliviousTranspose2<tbb::auto_partitioner>(N, a, b, gs);

    setArray(N, a);
    setArray(N, b);
    obliviousTranspose2<tbb::simple_partitioner>(N, a, b, gs);
    double t2d_simple = obliviousTranspose2<tbb::simple_partitioner>(N, a, b, gs);

    std::cout << gs 
              << ", " << to 
              << ", " << t1d_auto 
              << ", " << t1d_simple 
              << ", " << t2d_auto
              << ", " << t2d_simple << std::endl;
  }
  return 0;
}

