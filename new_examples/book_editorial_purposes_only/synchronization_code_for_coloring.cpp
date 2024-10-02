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


for (int i = 0; i < N; ++i) hist[image[i]]++;


// my_mutex_t my_mutex; NOT here! Moved to the body! Yikes!
parallel_for(tbb :: blocked_range<size_t>{0, image.size()},
  [&](const tbb :: blocked_range<size_t>& r)
  {
    my_mutex_t my_mutex;
    my_mutex_t :: scoped_lock my_lock{my_mutex};
    for (size_t i = r.begin(); i < r.end(); ++i)
      hist_p[image[i]]++;
  });


  using my_mutex_t = speculative_spin_mutex;


using rwmutex_t = spin_rw_mutex;
rwmutex_t my_mutex;
{
  rwmutex_t :: scoped_lock my_lock{my_mutex, /*is_writer =*/false};
  // A reader lock is acquired so multiple
  // concurrent reads are allowed
}


bool success=my_lock.upgrade_to_writer();


#include <atomic>
std :: atomic<int> counter;
counter++;



if (--refCount == 0) { ... /* body */ ... };


tbb::atomic<uint_32_t> v;

;


ov=v; if (ov == old_v) v=new_v; return ov;

;

void fetch_and_triple(std::atomic<uint32_t>& v)
{
  uint32_t old_v;
  do {
    old_v=v; //take a snapshot
  } while (v.compare_exchange_weak(old_v, old_v * 3)!=old_v);
}



template<typename Func> void combine_each(Func f)

;

template<typename Func> T combine(Func f)

;


// see #14
[num_bins](vector_t a, const vector_t & b) -> vector_t {
    for(int i=0; i<num_bins; ++i) a[i] += b[i];
    return a;
  }

;


sum = 0;
for (int i = 0; i < N; ++i) sum += vec[i];



;

long long sum_g = 0;
parallel_for(tbb :: blocked_range<size_t>{0, N},
  [&](const tbb :: blocked_range<size_t>& r)
  {
    for (int i=r.begin(); i<r.end(); ++i) sum_g+=vec[i];
  });


;

parallel_for(tbb :: blocked_range<size_t>{0, N},
  [&](const tbb :: blocked_range<size_t>& r){
    my_mutex_t :: scoped_lock mylock{my_mutex};
    for (int i=r.begin(); i<r.end(); ++i) sum_g+=vec[i];
  });



;




parallel_for(tbb :: blocked_range<size_t>{0, N},
  [&](const tbb :: blocked_range<size_t>& r){
      for (int i=r.begin(); i<r.end(); ++i){
        my_mutex_t :: scoped_lock mylock{my_mutex};
        sum_g+=vec[i];
      }
  });


;




std :: atomic<long long> sum_a{0};
parallel_for(tbb :: blocked_range<size_t>{0, N},
  [&](const tbb :: blocked_range<size_t>& r)
  {
    for (int i=r.begin(); i<r.end(); ++i) sum_a+=vec[i];
  });



;


using priv_s_t = tbb :: enumerable_thread_specific<long long>;
priv_s_t priv_s{0};
parallel_for(tbb :: blocked_range<size_t>{0, N},
  [&](const tbb :: blocked_range<size_t>& r)
  { priv_s_t :: reference my_s = priv_s.local();
    for (int i=r.begin(); i<r.end(); ++i) my_s+=vec[i];
  });
  long long sum_p = 0;
  for (auto& i:priv_s) {sum_p+=i;}


;




sum_p = parallel_reduce(tbb :: blocked_range<size_t>{0, N}, 0,
[&](const tbb :: blocked_range<size_t>&r, const long long &mysum)
  {
    long long res = mysum;
    for (int i=r.begin(); i<r.end(); ++i) res+=vec[i];
    return res;
  },
[&](const long long& a, const long long& b)
  {
    return a+b;
  });




;



