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

// this pseudo-code was used in the book "Today's TBB" (2015)
// it serves no other purpose other than to be here to verify compilation,
// and provide consist code coloring for the book


struct atom_bin {
 alignas(std::hardware_destructive_interference_size) 
   std::atomic<int> count;
};
std::vector<atom_bin, tbb::cache_aligned_allocator<atom_bin>>
  hist_p(num_bins);




#define TBB_PREVIEW_MEMORY_POOL 1
#include <tbb/memory_pool.h>



