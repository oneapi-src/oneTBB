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


int main() {
  int N = 2<<13;
  double *a = new double[N*N];
  double *b = new double[N*N];

  std::cout << "grainsize, oblivious time" << std::endl;
  for (int gs = 1; gs < 2048; gs *= 2) {
    setArray(N, a);
    setArray(N, b);
    obliviousTranspose(N, a, b, gs);
    double to = obliviousTranspose(N, a, b, gs);
    checkTranspose(N, b);
    std::cout << gs << ", " << to << std::endl;
  }

  delete [] a;
  delete [] b;
  return 0;
}

