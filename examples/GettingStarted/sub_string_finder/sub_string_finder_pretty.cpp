/*
    Copyright (c) 2005-2016 Intel Corporation

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
#include <algorithm>

#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"

using namespace tbb;
static const size_t N = 9;

class SubStringFinder {
 const std::string str;
 size_t *max_array;
 size_t *pos_array;
 public:
 void operator() ( const blocked_range<size_t>& r ) const {
  for ( size_t i = r.begin(); i != r.end(); ++i ) {
   size_t max_size = 0, max_pos = 0;
   for (size_t j = 0; j < str.size(); ++j) 
    if (j != i) {
     size_t limit = str.size()-( i > j ? i : j );
     for (size_t k = 0; k < limit; ++k) {
      if (str[i + k] != str[j + k]) break;
      if (k+1 > max_size) {
       max_size = k+1;
       max_pos = j;
      }
     }
    }
   max_array[i] = max_size;
   pos_array[i] = max_pos;
  }
 }
 SubStringFinder(std::string &s, size_t *m, size_t *p) :
  str(s), max_array(m), pos_array(p) { }
};

int main(int argc, char *argv[]) {
 

 std::string str[N] = { std::string("a"), std::string("b") };
 for (size_t i = 2; i < N; ++i) str[i] = str[i-1]+str[i-2];
 std::string &to_scan = str[N-1]; 
 size_t num_elem = to_scan.size();
 std::cout << "String to scan: " << to_scan << std::endl;

 size_t *max = new size_t[num_elem];
 size_t *pos = new size_t[num_elem];

 parallel_for(blocked_range<size_t>(0, num_elem, 100),
       SubStringFinder( to_scan, max, pos ) );

 for (size_t i = 0; i < num_elem; ++i) {
   for (size_t j = 0; j < num_elem; ++j) {
     if (j >= i && j < i + max[i]) std::cout << "_";
     else std::cout << " ";
   } 
   std::cout << std::endl << to_scan << std::endl;

   for (size_t j = 0; j < num_elem; ++j) {
     if (j >= pos[i] && j < pos[i] + max[i]) std::cout << "*";
     else std::cout << " ";
   }
   std::cout << std::endl;
 }
 delete[] max;
 delete[] pos;
 return 0;
}

