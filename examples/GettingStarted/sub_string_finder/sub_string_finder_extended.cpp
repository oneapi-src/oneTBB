/*
    Copyright 2005-2016 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks. Threading Building Blocks is free software;
    you can redistribute it and/or modify it under the terms of the GNU General Public License
    version 2  as  published  by  the  Free Software Foundation.  Threading Building Blocks is
    distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See  the GNU General Public License for more details.   You should have received a copy of
    the  GNU General Public License along with Threading Building Blocks; if not, write to the
    Free Software Foundation, Inc.,  51 Franklin St,  Fifth Floor,  Boston,  MA 02110-1301 USA

    As a special exception,  you may use this file  as part of a free software library without
    restriction.  Specifically,  if other files instantiate templates  or use macros or inline
    functions from this file, or you compile this file and link it with other files to produce
    an executable,  this file does not by itself cause the resulting executable to be covered
    by the GNU General Public License. This exception does not however invalidate any other
    reasons why the executable file might be covered by the GNU General Public License.
*/

#if __TBB_MIC_OFFLOAD
#pragma offload_attribute (push,target(mic))
#endif // __TBB_MIC_OFFLOAD

#include <iostream>
#include <string>
#include <algorithm>

#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"
#include "tbb/tick_count.h"

#if __TBB_MIC_OFFLOAD
#pragma offload_attribute (pop)

class __declspec(target(mic)) SubStringFinder;
#endif // __TBB_MIC_OFFLOAD

using namespace tbb;
using namespace std;
static const size_t N = 22;

void SerialSubStringFinder ( const string &str, size_t *max_array, size_t *pos_array) {
 for ( size_t i = 0; i < str.size(); ++i ) {
   size_t max_size = 0, max_pos = 0;
   for (size_t j = 0; j < str.size(); ++j)
    if (j != i) {
     size_t limit = str.size()-( i > j ? i : j );
     for (size_t k = 0; k < limit; ++k) {
      if (str[i + k] != str[j + k]) break;
      if (k > max_size) {
       max_size = k;
       max_pos = j;
      }
     }
    }
   max_array[i] = max_size;
   pos_array[i] = max_pos;
  }
}

class SubStringFinder {
 const string str;
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
      if (k > max_size) {
       max_size = k;
       max_pos = j;
      }
     }
    }
   max_array[i] = max_size;
   pos_array[i] = max_pos;
  }
 }
 SubStringFinder(string &s, size_t *m, size_t *p) : 
  str(s), max_array(m), pos_array(p) { }
};

int main(int argc, char *argv[]) {
 

 string str[N] = { string("a"), string("b") };
 for (size_t i = 2; i < N; ++i) str[i] = str[i-1]+str[i-2];
 string &to_scan = str[N-1]; 
 size_t num_elem = to_scan.size();

 size_t *max = new size_t[num_elem];
 size_t *max2 = new size_t[num_elem];
 size_t *pos = new size_t[num_elem];
 size_t *pos2 = new size_t[num_elem];
 cout << " Done building string." << endl;

 
 tick_count serial_t0 = tick_count::now();
 SerialSubStringFinder(to_scan, max2, pos2);
 tick_count serial_t1 = tick_count::now();
 cout << " Done with serial version." << endl;

 tick_count parallel_t0 = tick_count::now();
 parallel_for(blocked_range<size_t>(0, num_elem, 100),
       SubStringFinder( to_scan, max, pos ) );
 tick_count parallel_t1 = tick_count::now();
 cout << " Done with parallel version." << endl;

 for (size_t i = 0; i < num_elem; ++i) {
   if (max[i] != max2[i] || pos[i] != pos2[i]) {
     cout << "ERROR: Serial and Parallel Results are Different!" << endl;
   }
 }
 cout << " Done validating results." << endl;

 cout << "Serial version ran in " << (serial_t1 - serial_t0).seconds() << " seconds" << endl
           << "Parallel version ran in " <<  (parallel_t1 - parallel_t0).seconds() << " seconds" << endl
           << "Resulting in a speedup of " << (serial_t1 - serial_t0).seconds() / (parallel_t1 - parallel_t0).seconds() << endl;

#if __TBB_MIC_OFFLOAD
 // Do offloadable version. Do the timing on host.
 size_t *max3 = new size_t[num_elem];
 size_t *pos3 = new size_t[num_elem];
 tick_count parallel_tt0 = tick_count::now();
 const char* to_scan_str = to_scan.c_str();  // Offload the string as a char array.
 #pragma offload target(mic) in(num_elem) in(to_scan_str:length(num_elem)) out(max3,pos3:length(num_elem))
 {
 string to_scan(to_scan_str, num_elem);      // Reconstruct the string in offloadable region.
                                             // Suboptimal performance because of making a copy from to_scan_str at creating to_scan.
 parallel_for(blocked_range<size_t>(0, num_elem, 100),
       SubStringFinder( to_scan, max3, pos3 ) );
 }
 tick_count parallel_tt1 = tick_count::now();
 cout << " Done with offloadable version." << endl;

 // Do validation of offloadable results on host.
 for (size_t i = 0; i < num_elem; ++i) {
   if (max3[i] != max2[i] || pos3[i] != pos2[i]) {
     cout << "ERROR: Serial and Offloadable Results are Different!" << endl;
   }
 }
 cout << " Done validating offloadable results." << endl;

 cout << "Offloadable version ran in " <<  (parallel_tt1 - parallel_tt0).seconds() << " seconds" << endl
           << "Resulting in a speedup of " << (serial_t1 - serial_t0).seconds() / (parallel_tt1 - parallel_tt0).seconds() << " of offloadable version" << endl;

 delete[] max3;
 delete[] pos3;
#endif // __TBB_MIC_OFFLOAD

 delete[] max;
 delete[] pos;
 delete[] max2;
 delete[] pos2;
 return 0;
}

