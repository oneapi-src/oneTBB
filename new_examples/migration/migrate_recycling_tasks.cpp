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

#include <atomic>
#include <vector>
#include <tbb/tbb.h>

const int block_size = 512;
using BlockIndex = std::pair<size_t, size_t>;

void serialFwdSub(std::vector<double>& x, 
                  const std::vector<double>& a, 
                  std::vector<double>& b) {
  const int N = x.size();
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < i; ++j) {
      b[i] -= a[j + i*N] * x[j];
    }
    x[i] = b[i] / a[i + i*N];
  }
}

static inline void 
computeBlock(int N, int r, int c, 
             std::vector<double>& x, 
             const std::vector<double>& a, 
             std::vector<double>& b);

void serialFwdSubTiled(std::vector<double>& x, 
                       const std::vector<double>& a, 
                       std::vector<double>& b) {
  const int N = x.size();
  const int num_blocks = N / block_size;

  for ( int r = 0; r < num_blocks; ++r ) {
    for ( int c = 0; c <= r; ++c ) {
      computeBlock(N, r, c, x, a, b);
    }
  }
}

#if TBB_VERSION_MAJOR > 2020
class FwdSubFunctor {
public:
  FwdSubFunctor(tbb::task_group& tg, 
                int N, int num_blocks, 
                const std::pair<size_t, size_t>& bi, 
                std::vector<double>& x, 
                const std::vector<double>& a, 
                std::vector<double>& b,
                std::vector<std::atomic<char>>& ref_count) : 
                my_tg(tg), my_N(N), my_num_blocks(num_blocks), 
                my_index(new BlockIndex{bi}), 
                my_x(x), my_a(a), my_b(b), my_ref_count(ref_count) {}

  void operator()() const {
    auto [r, c] = *my_index;
    computeBlock(my_N, r, c, my_x, my_a, my_b);
    // add successor to right if ready
    bool recycle_as_c1 = false;
    if (c + 1 <= r && --my_ref_count[r*my_num_blocks + c + 1] == 0) {
      recycle_as_c1 = true;
    }
    // add succesor below if ready
    if (r + 1 < (size_t)my_num_blocks && --my_ref_count[(r+1)*my_num_blocks + c] == 0) {
      if (recycle_as_c1) {
        my_tg.run(FwdSubFunctor{my_tg, my_N, my_num_blocks, BlockIndex(r+1, c), my_x, my_a, my_b, my_ref_count});
      } else {
        *my_index = BlockIndex{r+1,c};
        my_tg.run(*this);
      }
    }
    if (recycle_as_c1) {
      *my_index = BlockIndex{r,c+1};
      my_tg.run(*this);
    }
  }

private:
  tbb::task_group& my_tg;
  const std::shared_ptr<BlockIndex> my_index;
  const int my_N, my_num_blocks;
  std::vector<double>& my_x;
  const std::vector<double>& my_a;
  std::vector<double>& my_b;
  std::vector<std::atomic<char>>& my_ref_count;
};

void parallelFwdSub(std::vector<double>& x, 
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
  tg.run(FwdSubFunctor{tg, N, num_blocks, top_left, x, a, b, ref_count});
  tg.wait();
}
#else
using RootTask = tbb::empty_task;

class FwdSubTask : public tbb::task {
public:
  FwdSubTask(RootTask& root, 
             int N, int num_blocks, 
             const std::pair<size_t, size_t>& bi, 
             std::vector<double>& x, 
             const std::vector<double>& a, 
             std::vector<double>& b,
             std::vector<std::atomic<char>>& ref_count) : 
             my_root(root), my_N(N), my_num_blocks(num_blocks), my_index(bi), 
             my_x(x), my_a(a), my_b(b), my_ref_count(ref_count) {}

  tbb::task* execute() override {
      auto [r, c] = my_index;
      computeBlock(my_N, r, c, my_x, my_a, my_b);
      // add successor to right if ready
      bool recycle_as_c1 = false;
      if (c + 1 <= r && --my_ref_count[r*my_num_blocks + c + 1] == 0) {
        recycle_as_c1 = true;
      }
      // add succesor below if ready
      if (r + 1 < (size_t)my_num_blocks && --my_ref_count[(r+1)*my_num_blocks + c] == 0) {
        if (recycle_as_c1) {
          FwdSubTask& child = *new(allocate_additional_child_of(my_root))
              FwdSubTask(my_root, my_N, my_num_blocks, {r+1, c}, 
                         my_x, my_a, my_b, my_ref_count);
          tbb::task::spawn(child);
        } else {
          my_index = {r+1, c};
          recycle_to_reexecute();
        }
      }
      if (recycle_as_c1) {
        my_index = {r, c+1};
        recycle_to_reexecute();
      }
      return nullptr;
  }

private:
  RootTask& my_root;
  BlockIndex my_index;
  const int my_N, my_num_blocks;
  std::vector<double>& my_x;
  const std::vector<double>& my_a;
  std::vector<double>& my_b;
  std::vector<std::atomic<char>>& my_ref_count;
};

void parallelFwdSub(std::vector<double>& x, 
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
  RootTask& root = *new(tbb::task::allocate_root()) RootTask{};
  root.set_ref_count(2);
  FwdSubTask& top_left_task = 
      *new(root.allocate_child()) FwdSubTask(root, N, num_blocks, top_left, x, a, b, ref_count);
  tbb::task::spawn(top_left_task);
  root.wait_for_all();
}
#endif

#include <iostream>

static std::vector<double> initForwardSubstitution(std::vector<double>& x, 
                                                   std::vector<double>& a, 
                                                   std::vector<double>& b);
static void warmupTBB();

int main() {
  const int N = 32768;

  std::vector<double> a(N*N);
  std::vector<double> b_serial(N), x_serial(N);
  std::vector<double> b_serial_tiled(N), x_serial_tiled(N);
  std::vector<double> b_tbb(N), x_tbb(N);

  auto x_gold = initForwardSubstitution(x_serial,a,b_serial);
  initForwardSubstitution(x_serial_tiled,a,b_serial_tiled);
  initForwardSubstitution(x_tbb,a,b_tbb);

  double serial_time = 0.0, serial_tiled_time = 0.0, tbb_time = 0.0;
  {
    tbb::tick_count t0 = tbb::tick_count::now();
    serialFwdSub(x_serial,a,b_serial);
    serial_time = (tbb::tick_count::now() - t0).seconds();
  }
  {
    tbb::tick_count t0 = tbb::tick_count::now();
    serialFwdSubTiled(x_serial_tiled,a,b_serial_tiled);
    serial_tiled_time = (tbb::tick_count::now() - t0).seconds();
  }
  warmupTBB();
  {
    tbb::tick_count t0 = tbb::tick_count::now();
    parallelFwdSub(x_tbb,a,b_tbb);
    tbb_time = (tbb::tick_count::now() - t0).seconds();
  }
  for (int i = 0; i < N; ++i) {
    if (x_serial[i] > 1.1*x_gold[i] || x_serial[i] < 0.9*x_gold[i]) {
        std::cerr << "serial did not validate at " << i << " " << x_serial[i] << " != " << x_gold[i] << std::endl;
    }
    if (x_serial_tiled[i] > 1.1*x_gold[i] || x_serial_tiled[i] < 0.9*x_gold[i]) {
        std::cerr << "serial tiled did not validate at " << i << " " << x_serial_tiled[i] << " != " << x_gold[i] << std::endl;
    }
    if (x_tbb[i] > 1.1*x_gold[i] || x_tbb[i] < 0.9*x_gold[i]) {
        std::cerr << "parallel did not validate at " << i << " " << x_tbb[i] << " != " << x_gold[i] << std::endl;
    }
  }
  std::cout << "serial_time == " << serial_time << " seconds" << std::endl;
  std::cout << "serial_tiled_time == " << serial_tiled_time << " seconds" << std::endl;
  std::cout << "tbb_time    == " << tbb_time << " seconds" << std::endl;
  std::cout << "speedup (serial_tiled_time/tbb_time) == " << serial_tiled_time/tbb_time << std::endl;
  return 0;
}

static std::vector<double> initForwardSubstitution(std::vector<double>& x, 
                                                   std::vector<double>& a, 
                                                   std::vector<double>& b) {
  const int N = x.size();
  for (int i = 0; i < N; ++i) {
    x[i] = 0;
    b[i] = i*i;
    for (int j = 0; j <= i; ++j) {
      a[j + i*N] = 1 + j*i;
    }
  }

  std::vector<double> b_tmp = b;
  std::vector<double> x_gold = x;
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < i; ++j) {
      b_tmp[i] -= a[j + i*N] * x_gold[j];
    }
    x_gold[i] = b_tmp[i] / a[i + i*N];
  }
  return x_gold;
}

static inline void 
computeBlock(int N, int r, int c,
             std::vector<double>& x, 
             const std::vector<double>& a, 
             std::vector<double>& b) {
  int i_start = r*block_size, i_end = i_start + block_size;
  int j_start = c*block_size, j_max = j_start + block_size - 1;
  for (int i = i_start; i < i_end; ++i) {
    int j_end = (i <= j_max) ? i : j_max + 1;
    for (int j = j_start; j < j_end; ++j) {
      b[i] -= a[j + i*N] * x[j];
    }
    if (j_end == i) {
      x[i] = b[i] / a[i + i*N];
    }
  }
}

static void warmupTBB() {
  // This is a simple loop that should get workers started.
  // oneTBB creates workers lazily on first use of the library
  // so this hides the startup time when looking at trivial
  // examples that do little real work. 
#if TBB_VERSION_MAJOR > 2020
  auto P = tbb::info::default_concurrency();
#else
  auto P = tbb::task_scheduler_init::default_num_threads();
#endif
  tbb::parallel_for(0, P, 
   [=](int) {
      tbb::tick_count t0 = tbb::tick_count::now();
      while ((tbb::tick_count::now() - t0).seconds() < 0.01);
    }
  );
}

