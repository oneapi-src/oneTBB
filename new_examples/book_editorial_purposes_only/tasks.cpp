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

long fib(long n) {
  if(n<2)
    return n;
  else
    return fib(n-1)+fib(n-2);
}


long parallel_fib(long n) {
  if(n<2) {
    return n;
  }
  else {
    long x, y;
    tbb::parallel_invoke([&]{x=parallel_fib(n-1);},
                         [&]{y=parallel_fib(n-2);});
    return x+y;
  }
}


long parallel_fib_cutoff(log n) {
  if(n<cutoff) {
    return fib(n);
  }
  else {
    long x, y;
    tbb::parallel_invoke([&]{x=parallel_fib_cutoff(n-1);},
                         [&]{y=parallel_fib_cutoff(n-2);});
    return x+y;
  }
}

// Defined in header <oneapi/tbb/task_group.h>

namespace oneapi {
namespace tbb {

    class task_group {
    public:
        task_group();
        task_group(task_group_context& context);

        ~task_group();

        template<typename Func>
        void run(Func&& f);

        template<typename Func>
        task_handle defer(Func&& f);

        void run(task_handle&& h);

        template<typename Func>
        task_group_status run_and_wait(const Func& f);

        task_group_status run_and_wait(task_handle&& h);

        task_group_status wait();
        void cancel();
    };

    bool is_current_task_group_canceling();

} // namespace tbb
} // namespace oneapi



long parallel_fib(long n) {
  if(n<cutoff) {
    return fib(n);
  }
  else {
    long x, y;
    tbb::task_group g;
    g.run([&]{x=parallel_fib(n-1);}); // spawn a task
    g.run([&]{y=parallel_fib(n-2);}); // spawn another task
    g.wait();                // wait for both tasks to complete
    return x+y;
  }
}





#include <tbb/tbb.h>

void f();

void serializedCallsToRun(tbb::task_group& g,
                          int depth) {
  int n = 1<<depth;
  for (int i = 1; i < n; ++i) {
    g.run([]() { f(); });
  }
}

void serialCalls(int depth) {
  tbb::task_group g;
  serializedCallsToRun(g, depth);
  g.wait();
}



#include <tbb/tbb.h>

void f();

void treeCallsToRun(tbb::task_group& g, int depth) {
  if (depth>1) {
    g.run([&g, depth]() { 
      treeCallsToRun(g, depth-1); 
    });
    g.run([&, depth]() { 
      treeCallsToRun(g, depth-1);
    });
  }
  f();
}

void treeCalls(int depth) {
  tbb::task_group g;
  treeCallsToRun(g, depth);
  g.wait();
}





tbb::task_handle make_task(tbb::task_group& g, long& r, long n);

long parallel_fib(long n) {
  if(n<cutoff) {
    return fib(n);
  }
  else {
    long x, y;
    tbb::task_group g;
    tbb::task_handle h1 = make_task(g, x, n-1);
    tbb::task_handle h2 = make_task(g, y, n-2);
    g.run(std::move(h1));
    g.run(std::move(h2));
    g.wait();                // wait for both tasks to complete
    return x+y;
  }
}

tbb::task_handle make_task(tbb::task_group& g, long& r, long n) {
  return g.defer([&r,n]{r=parallel_fib(n);});
}







void parallelFwdSubTaskGroup(std::vector<double>& x, 
                             const std::vector<double>& a, 
                             std::vector<double>& b) {
  const int N = x.size();
  const int num_blocks = N / block_size;

  // create reference counts
  std::vector<std::atomic<char>> ref_count(num_blocks*num_blocks);
  ref_count[0] = 0;
  for (int r = 1; r < num_blocks; ++r) {
    ref_count[r*num_blocks] = 1;
    for (int c = 1; c < r; ++c) {
      ref_count[r*num_blocks + c] = 2;
    }
    ref_count[r*num_blocks + r] = 1;
  }

  BlockIndex top_left(0,0);

  tbb::task_group tg;
  tg.run([&]() {
    fwdSubTGBody(tg, N, num_blocks, top_left, x, a, b, ref_count);
  });
  tg.wait();
}






void fwdSubTGBody(tbb::task_group& tg,
                int N, int num_blocks, 
                const std::pair<size_t, size_t> bi, 
                std::vector<double>& x, 
                const std::vector<double>& a, 
                std::vector<double>& b,
                std::vector<std::atomic<char>>& ref_count) {
  auto [r, c] = bi;
  computeBlock(N, r, c, x, a, b);
  // add successor to right if ready
  if (c + 1 <= r && --ref_count[r*num_blocks + c + 1] == 0) {
    tg.run([&, N, num_blocks, r, c]() { 
      fwdSubTGBody(tg, N, num_blocks, 
                   BlockIndex(r, c+1), x, a, b, ref_count);
    });
  }
  // add succesor below if ready
  if (r + 1 < (size_t)num_blocks 
      && --ref_count[(r+1)*num_blocks + c] == 0) {
    tg.run([&, N, num_blocks, r, c]() { 
      fwdSubTGBody(tg, N, num_blocks, 
                   BlockIndex(r+1, c), x, a, b, ref_count);
    });
  }
}


using tbb::task::suspend_point = /* implementation-defined */;
template < typename Func > void tbb::task::suspend( Func );
void tbb::task::resume( tbb::task::suspend_point );




tbb::parallel_for(0, N, [&](int) {
  tbb::task::suspend(
    [&] (tbb::task::suspend_point tag) {
      async_activity.submit(tag); 
    });
   // once resumed will start executing here:
   next_thing_to_do_after_resumed();
});




#include <iostream>
#include <utility>
#include <sycl/sycl.hpp>
#include <tbb/tbb.h>

int main() {
  const int N = 1000;
  int num_iterations = 3;

  try {
    sycl::queue sycl_q;

    tbb::parallel_for(0, num_iterations, 
      [&](int id) {
        int *a = sycl::malloc_shared<int>(N, sycl_q);
        tbb::task::suspend([=,&sycl_q](tbb::task::suspend_point tag) {
          auto sycl_event = sycl_q.fill(a, id, N);
          sycl_q.submit([=](sycl::handler& sycl_h) {
            sycl_h.depends_on(sycl_event); // only run after e is done
            sycl_h.host_task([tag]() { 
              tbb::task::resume(tag);
            });
          });
        });
        if (std::any_of(a, a+N, [id](int i) { return i != id; })) {
          std::printf("ERROR: unexpected fill result\n"); 
        } else {
          std::printf("Well-formed fill for id %d\n", id);
        }
      }
    );
  } catch (const sycl::exception&) {
    std::cout << "No CPU SYCL device on this platform\n";
  }
  return 0;
}



