/*
  Copyright (c) 2018 Intel Corporation

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

#include "03_container_data.hpp"

#include <stdexcept>
#include <iostream>
#include <cstdlib>
#include <cassert>
#include <vector>

//#include <unordered_map>
#include <map>

#include <tbb/tbb.h>
#include <tbb/spin_mutex.h>


struct hash {
  std::size_t operator()(const std::pair<const char*,const char*>& a) const {
    static const std::size_t M = 4 * sizeof(std::size_t);
    const std::size_t i = a.first - *names_first, j = a.second - *names_last;    
    return (i << M) | j;
  }
};


struct less {
  bool operator()(const std::pair<const char*,const char*>& a, const std::pair<const char*,const char*>& b) const {
    const int u = std::strcmp(a.first, b.first), v = std::strcmp(a.second, b.second);
    return u < 0 || (0 == u && v < 0);
  }
};


struct equal {
  bool operator()(const std::pair<const char*,const char*>& a, const std::pair<const char*,const char*>& b) const {
    const int u = std::strcmp(a.first, b.first), v = std::strcmp(a.second, b.second);
    return 0 == u && 0 == v;
  }
};


int main(int argc, char* argv[])
{
  try {
    using namespace tbb;
    typedef std::pair<const char*,const char*> key_type;
    //typedef std::map<key_type,std::size_t,less> map_type;
    //typedef std::unordered_map<key_type,std::size_t,hash,equal> map_type;
    typedef concurrent_unordered_map<key_type,atomic<std::size_t>,hash,equal> map_type;

    const std::size_t size = 1 < argc ? std::atol(argv[1]) : 100;
    map_type result_serial, result_parallel;
    atomic<std::size_t> zero;
    zero = 0;

    std::cout << "Generating initial data...";
    const std::size_t M = sizeof(names_first) / sizeof(*names_first);
    const std::size_t N = sizeof(names_last) / sizeof(*names_last);
    std::vector<key_type> names;
    for (std::size_t i = 0; i < size; ++i) {
      names.push_back(std::make_pair(names_first[std::rand()%M], names_last[std::rand()%N]));
    }
    std::cout << "    size: " << size << '\n';

    std::cout << "Running serial algorithm...";
    tick_count start = tick_count::now();
    for (std::size_t i = 0; i < size; ++i) {
      ++(result_serial.insert(std::make_pair(names[i], zero)).first);
    }
    const double time_serial = (tick_count::now() - start).seconds();
    std::cout << "   time: " << time_serial << " s\n";

    std::cout << "Running parallel algorithm...";
    spin_mutex lock;
    start = tick_count::now();
    parallel_for(static_cast<std::size_t>(0), size, [&](std::size_t i) {
      //tbb::spin_mutex::scoped_lock scoped_lock(lock);
      ++(result_parallel.insert(std::make_pair(names[i], zero)).first);
    });
    const double time_parallel = (tick_count::now() - start).seconds();
    std::cout << " time: " << time_parallel << " s\n";

    std::cout << "Verifying results... ";
    if (result_serial.size() != result_parallel.size()) {
      throw std::runtime_error("size does not match!");
    }
    for (std::size_t i = 0; i < size; ++i) {
      if (result_serial[names[i]] != result_parallel[names[i]]) {
        throw std::runtime_error("entry does not match!");
      }
    }
    if (0 != time_parallel) {
      std::cout << "      speedup: " << (time_serial / time_parallel) << "x";
    }
    std::cout << '\n';

    return EXIT_SUCCESS;
  }
  catch(const std::exception& e) {
    std::cerr << "Error: " << e.what() << '\n';
    return EXIT_FAILURE;
  }
  catch(...) {
    std::cerr << "Error: unknown exception caught!\n";
    return EXIT_FAILURE;
  }
}
