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
#include <tbb/tbb.h>

int main(int argc, char** argv) {

  {
    tbb::mutex my;
    std::cout << "tbb::mutex sizeof = " << sizeof(my) << std::endl;
  }
  
  {
    tbb::null_mutex my;
    std::cout << "tbb::null_mutex sizeof = " << sizeof(my) << std::endl;
  }
  
  {
    tbb::rw_mutex  my;
    std::cout << "tbb::rw_mutex  sizeof = " << sizeof(my) << std::endl;
  }
  
  {
    tbb::null_rw_mutex my;
    std::cout << "tbb::null_rw_mutex sizeof = " << sizeof(my) << std::endl;
  }
  
  {
    tbb::queuing_mutex my;
    std::cout << "tbb::queuing_mutex sizeof = " << sizeof(my) << std::endl;
  }
  
  {
    tbb::queuing_rw_mutex my;
    std::cout << "tbb::queuing_rw_mutex sizeof = " << sizeof(my) << std::endl;
  }
  
  {
    tbb::speculative_spin_mutex my;
    std::cout << "tbb::speculative_spin_mutex sizeof = " << sizeof(my) << std::endl;
  }
  
  {
    tbb::speculative_spin_rw_mutex my;
    std::cout << "tbb::speculative_spin_rw_mutex sizeof = " << sizeof(my) << std::endl;
  }
  
  {
    tbb::spin_mutex my;
    std::cout << "tbb::spin_mutex sizeof = " << sizeof(my) << std::endl;
  }
  
  {
    tbb::spin_rw_mutex  my;
    std::cout << "tbb::spin_rw_mutex  sizeof = " << sizeof(my) << std::endl;
  }
  
  {
    tbb::spin_rw_mutex  my;
    std::cout << "tbb::spin_rw_mutex  sizeof = " << sizeof(my) << std::endl;
  }
  
  {
    std::mutex my;
    std::cout << "std::mutex sizeof = " << sizeof(my) << std::endl;
  }
  
  {
    std::recursive_mutex my;
    std::cout << "std::recursive_mutex sizeof = " << sizeof(my) << std::endl;
  }
  
  {
    std::recursive_timed_mutex my;
    std::cout << "std::recursive_timed_mutex sizeof = " << sizeof(my) << std::endl;
  }
  
  {
    std::shared_mutex my;
    std::cout << "std::shared_mutex sizeof = " << sizeof(my) << std::endl;
  }
  
  {
    std::shared_timed_mutex my;
    std::cout << "std::shared_timed_mutex sizeof = " << sizeof(my) << std::endl;
  }
  
  {
    std::timed_mutex my;
    std::cout << "std::timed_mutex sizeof = " << sizeof(my) << std::endl;
  }
  
  return 0;
}
