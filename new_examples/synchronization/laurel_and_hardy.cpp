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
#include <atomic>
#include <tbb/tbb.h>

#define THREADS 8
//#define SKIP_LAUREL_AND_HARDY

int main(int argc, char** argv) {
  size_t N = 1000000000;

  std::cout<<"Par_count with N: " << N << std::endl;

  //  std::cout<<"Threads = " << tbb::info::default_concurrency() << std::endl;
  tbb::global_control gc(tbb::global_control::max_allowed_parallelism, THREADS);
  //  std::cout<<"Threads = " << tbb::info::default_concurrency() << std::endl;
  std::cout<<"Threads = " << THREADS << std::endl;

  //for (int test = 0; test < 10 ; ++test)
  //{

  std::vector<uint8_t> vec(N);
  //std::iota(vec.begin(), vec.end(), 0);
  //std::generate(vec.begin(), vec.end(), [n = 0] () mutable { return n++; });
  std::generate(vec.begin(), vec.end(), [](){ return 1; });

  // Serial execution
  long long sum=0;
  tbb::tick_count t0_s = tbb::tick_count::now();
  for_each(vec.begin(), vec.end(), [&](uint8_t i){sum+=i;});
  tbb::tick_count t1_s = tbb::tick_count::now();
  double t_s = (t1_s - t0_s).seconds();
  std::cout << "Serial \t result: " << sum << " Time: " << t_s << std::endl;

  // Parallel execution (wrong)
  tbb::tick_count t0_p = tbb::tick_count::now();
  long long sum_g = 0;
  parallel_for(tbb::blocked_range<size_t>{0, N},
    [&](const tbb::blocked_range<size_t>& r)
    {
      for(int i=r.begin(); i<r.end(); ++i) sum_g+=vec[i];
    });//,tbb::auto_partitioner());
  tbb::tick_count t1_p = tbb::tick_count::now();
  double t_p = (t1_p - t0_p).seconds();
  std::cout << "Parallel (Mistaken) \t result: " << sum_g;
  if(sum_g != sum) std::cout << " (INCORRECT!) ";
  std::cout << " Time: " << t_p ;
  std::cout << "\tSpeedup: " << t_s/t_p << std::endl;

#ifndef SKIP_LAUREL_AND_HARDY
  // Parallel execution (coarse)
  //using myMutex_t = spin_mutex;
  using my_mutex_t = tbb::queuing_mutex;
  my_mutex_t my_mutex;
  t0_p = tbb::tick_count::now();
  sum_g = 0;
  parallel_for(tbb::blocked_range<size_t>{0, N},
    [&](const tbb::blocked_range<size_t>& r){
      my_mutex_t::scoped_lock mylock{my_mutex};
      for(int i=r.begin(); i<r.end(); ++i) sum_g+=vec[i];
    });//,tbb::auto_partitioner());
  t1_p = tbb::tick_count::now();
  t_p = (t1_p - t0_p).seconds();
  std::cout << "Parallel (Hardy) \t result: " << sum_g;
  if(sum_g != sum) std::cout << " (INCORRECT!) ";
  std::cout << " Time: " << t_p ;
  std::cout << "\tSpeedup: " << t_s/t_p << std::endl;

  // Parallel execution (fine)
  t0_p = tbb::tick_count::now();
  sum_g = 0;
  parallel_for(tbb::blocked_range<size_t>{0, N},
    [&](const tbb::blocked_range<size_t>& r)
    {
      for(int i=r.begin(); i<r.end(); ++i) {
        my_mutex_t::scoped_lock mylock{my_mutex};
        sum_g+=vec[i];}
    });//,tbb::auto_partitioner());
  t1_p = tbb::tick_count::now();
  t_p = (t1_p - t0_p).seconds();
  if(sum_g != sum) std::cout << "INCORRECT" <<std::endl;
  std::cout << "Parallel (Laurel) \t result: " << sum_g;
  if(sum_g != sum) std::cout << " (INCORRECT!) ";
  std::cout << " Time: " << t_p ;
  std::cout << "\tSpeedup: " << t_s/t_p << std::endl;
#endif
  
  // Parallel execution (atomic)
  t0_p = tbb::tick_count::now();
  std::atomic<long long> sum_a{0};
  parallel_for(tbb::blocked_range<size_t>{0, N},
    [&](const tbb::blocked_range<size_t>& r)
    {
      std::for_each(vec.data() + r.begin(), vec.data() + r.end(),
      [&](const int i){sum_a+=i;});
      //for(int i=r.begin(); i<r.end(); ++i) sum_a+=vec[i];
    });//,tbb::auto_partitioner());
  t1_p = tbb::tick_count::now();
  t_p = (t1_p - t0_p).seconds();
  std::cout << "Parallel (Nuclear) \t result: " << sum_a ;
  if(sum_a != sum) std::cout << "INCORRECT";
  std::cout << " Time: " << t_p ;
  std::cout << "\tSpeedup: " << t_s/t_p << std::endl;

  // Parallel execution (TLS - ETS)
  t0_p = tbb::tick_count::now();
  using priv_s_t = tbb::enumerable_thread_specific<long long>;
  priv_s_t priv_s{0};
  parallel_for(tbb::blocked_range<size_t>{0, N},
    [&](const tbb::blocked_range<size_t>& r)
    { priv_s_t::reference my_s = priv_s.local();
      for(int i=r.begin(); i<r.end(); ++i) my_s+=vec[i];
    });
  long long sum_p = 0;
  for (auto& i:priv_s) {sum_p+=i;}
  t1_p = tbb::tick_count::now();
  t_p = (t1_p - t0_p).seconds();
  std::cout << "Parallel (Local ETS) \t result: " << sum_p ;
  if(sum_p != sum) std::cout << "INCORRECT";
  std::cout << " Time: " << t_p ;
  std::cout << "\tSpeedup: " << t_s/t_p << std::endl;

  // Parallel execution (TLS - combinable)
  t0_p = tbb::tick_count::now();
  tbb::combinable<long long> priv_sum{[](){return 0;}};
  parallel_for(tbb::blocked_range<size_t>{0, N},
    [&](const tbb::blocked_range<size_t>& r)
    { long long& my_s = priv_sum.local();
      for(int i=r.begin(); i<r.end(); ++i) my_s+=vec[i];
    });
  sum_p =
  priv_sum.combine([](long long a, long long b) -> long long
  { return a+b;});
  t1_p = tbb::tick_count::now();
  t_p = (t1_p - t0_p).seconds();
  std::cout << "Parallel (Local) \t result: " << sum_p ;
  if(sum_p != sum) std::cout << "INCORRECT";
  std::cout << " Time: " << t_p ;
  std::cout << "\tSpeedup: " << t_s/t_p << std::endl;

  // Parallel execution (reduction)
  t0_p = tbb::tick_count::now();
  sum_p =
  parallel_reduce(tbb::blocked_range<size_t>{0, N},
    /*identity*/  0,
    [&](const tbb::blocked_range<size_t>& r, const long long& mysum )// -> long long
    {
      long long res = mysum;
      for(int i=r.begin(); i<r.end(); ++i) res+=vec[i];
      return res;
    },
    [&](const long long& a, const long long& b) //-> long long
    {
      return a+b;
    });//,tbb::auto_partitioner());
    t1_p = tbb::tick_count::now();
    t_p = (t1_p - t0_p).seconds();
    std::cout << "Parallel (Wise) \t result: " << sum_p;
    if(sum_p != sum) std::cout << " (INCORRECT!) ";
    std::cout << " Time: " << t_p ;
    std::cout << "\tSpeedup: " << t_s/t_p << std::endl;

    //}
    return 0;
  }

/*
sixteen-core (limited to eight threads in code) circa 2024
Par_count with N: 1000000000
Serial   result: 1000000000 Time: 0.17135
Parallel (Mistaken)      result: 999910711 (INCORRECT!)  Time: 0.0297814        Speedup: 5.75358
Parallel (Hardy)         result: 1000000000 Time: 0.210511      Speedup: 0.813971
Parallel (Laurel)        result: 1000000000 Time: 212.707       Speedup: 0.000805566
Parallel (Nuclear)       result: 1000000000 Time: 18.0024       Speedup: 0.00951815
Parallel (Local ETS)     result: 1000000000 Time: 0.0271777     Speedup: 6.30479
Parallel (Local)         result: 1000000000 Time: 0.0272057     Speedup: 6.2983
Parallel (Wise)          result: 1000000000 Time: 0.0271524     Speedup: 6.31067

quad-core circa 2019
Par_count with N: 1000000000 and nth: 4
Serial 	 result: 1000000000 Time: 0.330783
Parallel (Mistaken) 	 result: 269205706 (INCORRECT!)  Time: 0.098092	Speedup: 3.37217
Parallel (Hardy) 	 result: 1000000000 Time: 0.35784	Speedup: 0.924388
Parallel (Laurel) 	 result: 1000000000 Time: 433.269	Speedup: 0.000763458
Parallel (Nuclear) 	 result: 1000000000 Time: 22.5906	Speedup: 0.0146425
Parallel (Local ETS) 	 result: 1000000000 Time: 0.177264	Speedup: 1.86605
Parallel (Local) 	 result: 1000000000 Time: 0.204069	Speedup: 1.62094
Parallel (Wise) 	 result: 1000000000 Time: 0.093545	Speedup: 3.53608
*/
