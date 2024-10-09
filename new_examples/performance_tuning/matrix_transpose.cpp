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
#include <string>
#include <tbb/tbb.h>

void matrixTranspose(int N, double *a, double *b) {
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
      b[j*N+i] = a[i*N+j];
    }
  }
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
   setArray(N, a);
   setArray(N, b);

   matrixTranspose(N, a, b);
   tbb::tick_count t0 = tbb::tick_count::now();
   matrixTranspose(N, a, b);
   double ts = (tbb::tick_count::now()-t0).seconds();   
   checkTranspose(N, b);
   std::cout << "Serial Time = " << ts << std::endl;
   delete [] a;
   delete [] b;
   return 0;
}

