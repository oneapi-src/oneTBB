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

#include <vector>
#include <iostream>
#include <algorithm>
#include <random>
#include <atomic>
#include <tbb/tick_count.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>

int main(int argc, char** argv) {

  long int n = 1000000000;
  int nth = 4;
  constexpr int num_bins = 256;

  // Initialize random number generator
  std::random_device seed;    // Random device seed
  std::mt19937 mte{seed()};   // mersenne_twister_engine
  std::uniform_int_distribution<> uniform{0,num_bins};
  // Initialize image  
  std::vector<uint8_t> image; // empty vector
  image.reserve(n);           // image vector prealocated
  std::generate_n(std::back_inserter(image), n,
                    [&] { return uniform(mte); }
                 );
  // Initialize histogram
  std::vector<int> hist(num_bins);

  // Serial execution
  tbb::tick_count t0 = tbb::tick_count::now();
  std::for_each(image.begin(), image.end(),
                [&](uint8_t i){hist[i]++;});
  tbb::tick_count t1 = tbb::tick_count::now();
  double t_serial = (t1 - t0).seconds();

  // Parallel execution
  using vector_t = std::vector<int>;
  using image_iterator = std::vector<uint8_t>::iterator;
  t0 = tbb::tick_count::now();
  vector_t hist_p = parallel_reduce (
    /*range*/    tbb::blocked_range<image_iterator>{image.begin(), image.end()},
    /*identity*/ vector_t(num_bins),
    // 1st Lambda: Parallel computation on private histograms
	[](const tbb::blocked_range<image_iterator>& r, vector_t v) {
	        std::for_each(r.begin(), r.end(),
	            [&v](uint8_t i) {v[i]++;});
	        return v;
	    },
	// 2nd Lambda: Parallel reduction of the private histograms
	[](vector_t a, const vector_t& b) -> vector_t {
	   std::transform(a.begin(),         // source 1 begin
	                  a.end(),           // source 1 end
	                  b.begin(),         // source 2 begin
	                  a.begin(),         // destination begin
	                  std::plus<int>() );// binary operation
	    return a;
	});
  t1 = tbb::tick_count::now();
  double t_parallel = (t1 - t0).seconds();

  std::cout << "Serial: "   << t_serial   << ", ";
  std::cout << "Parallel: " << t_parallel << ", ";
  std::cout << "Speed-up: " << t_serial/t_parallel << std::endl;

  if (hist != hist_p)
      std::cerr << "Parallel computation failed!!" << std::endl;

  return 0;
}
