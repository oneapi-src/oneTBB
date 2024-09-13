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
#include <vector>
#include <tbb/tbb.h>

using Node = tbb::flow::continue_node<tbb::flow::continue_msg>;
using NodePtr = std::shared_ptr<Node>;

NodePtr createNode(tbb::flow::graph& g, 
                   int r, int c, int block_size, 
                   std::vector<double>& x, 
                   const std::vector<double>& a, 
                   std::vector<double>& b);

void addEdges(std::vector<NodePtr>& nodes, 
              int r, int c, int block_size, int num_blocks);

void graphFwdSub(std::vector<double>& x, 
                 const std::vector<double>& a, 
                 std::vector<double>& b) {
  const int N = x.size();
  const int block_size = 1024;
  const int num_blocks = N / block_size;

  std::vector<NodePtr> nodes(num_blocks*num_blocks);
  tbb::flow::graph g;
  for (int r = num_blocks - 1; r >= 0; --r) {
    for (int c = r; c >= 0; --c) {
      nodes[r*num_blocks + c] = createNode(g, r, c, block_size, x, a, b);;
      addEdges(nodes, r, c, block_size, num_blocks);
    }
  }
  nodes[0]->try_put(tbb::flow::continue_msg{});
  g.wait_for_all();
}

std::vector<double> initForwardSubstitution(std::vector<double>& x, 
                                            std::vector<double>& a, 
                                            std::vector<double>& b);

static void warmupTBB();

void serialFwdSub(std::vector<double>& x, 
                  const std::vector<double>& a,
                  std::vector<double>& b);

void parReduceFwdSub(std::vector<double>& x, 
                     const std::vector<double>& a, 
                     std::vector<double>& b);

int main() {
  const int N = 32768;

  std::vector<double> serial_a(N*N);
  std::vector<double> serial_b(N);
  std::vector<double> serial_x(N);

  auto x_gold = initForwardSubstitution(serial_x,serial_a,serial_b);

  std::vector<double> reduce_a(serial_a);
  std::vector<double> reduce_b(serial_b);
  std::vector<double> reduce_x(N);
  std::vector<double> graph_a(serial_a);
  std::vector<double> graph_b(serial_b);
  std::vector<double> graph_x(N);

  warmupTBB();
  double serial_time = 0.0;
  {
    tbb::tick_count t0 = tbb::tick_count::now();
    serialFwdSub(serial_x,serial_a,serial_b);
    serial_time = (tbb::tick_count::now() - t0).seconds();
  }
  warmupTBB();
  double reduce_time = 0.0;
  {
    tbb::tick_count t0 = tbb::tick_count::now();
    parReduceFwdSub(reduce_x,reduce_a,reduce_b);
    reduce_time = (tbb::tick_count::now() - t0).seconds();
  }
  warmupTBB();
  double graph_time = 0.0;
  {
    tbb::tick_count t0 = tbb::tick_count::now();
    graphFwdSub(graph_x,graph_a,graph_b);
    graph_time = (tbb::tick_count::now() - t0).seconds();
  }

  for (int i = 0; i < N; ++i) {
    if (serial_x[i] > 1.1*x_gold[i] || serial_x[i] < 0.9*x_gold[i]) 
        std::cerr << "serial error too large at " << i 
                  << " " << serial_x[i] << " != " << x_gold[i] << std::endl;
    if (reduce_x[i] > 1.1*x_gold[i] || reduce_x[i] < 0.9*x_gold[i]) 
        std::cerr << "parallel_reduce error too large at " << i 
                  << " " << reduce_x[i] << " != " << x_gold[i] << std::endl;
    if (graph_x[i] > 1.1*x_gold[i] || graph_x[i] < 0.9*x_gold[i]) 
        std::cerr << "graph error too large at " << i 
                  << " " << graph_x[i] << " != " << x_gold[i] << std::endl;
  }
  std::cout << "serial tiled time == " << serial_time << " seconds" << std::endl;
  std::cout << "parallel_reduce time == " << reduce_time << " seconds" << std::endl;
  std::cout << "graph time == " << graph_time << " seconds" << std::endl;
  std::cout << "\nparallel_reduce speedup == " << serial_time/reduce_time << std::endl;
  std::cout << "graph speedup == " << serial_time/graph_time << std::endl;
  return 0;
}

NodePtr createNode(tbb::flow::graph& g, 
                   int r, int c, int block_size, 
                   std::vector<double>& x, 
                   const std::vector<double>& a, 
                   std::vector<double>& b) {
  const int N = x.size();
  return std::make_shared<Node>(g,
    [r, c, block_size, N, &x, &a, &b] (const tbb::flow::continue_msg& msg) {
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
      return msg;
    }
  );
}

void addEdges(std::vector<NodePtr>& nodes, 
              int r, int c, int block_size, int num_blocks) {
  NodePtr np = nodes[r*num_blocks + c];
  if (c + 1 < num_blocks && r != c)
    tbb::flow::make_edge(*np, *nodes[r*num_blocks + c + 1]);
  if (r + 1 < num_blocks)
    tbb::flow::make_edge(*np, *nodes[(r + 1)*num_blocks + c]);
}


void parReduceFwdSub(std::vector<double>& x, 
                     const std::vector<double>& a, 
                     std::vector<double>& b) {
  const int N = x.size();
  for (int i = 0; i < N; ++i) {
    b[i] -= tbb::parallel_reduce(tbb::blocked_range<int>{0,i}, 0.0,
      [&a, &x, i, N] (const tbb::blocked_range<int>& b, double init) -> double {
        for (int j = b.begin(); j < b.end(); ++j) {
          init += a[j + i*N] * x[j];
        }
        return init;
      },
      std::plus<double>() 
    );
    x[i] = b[i] / a[i + i*N];
  }
}

void serialFwdSub(std::vector<double>& x, 
                  const std::vector<double>& a,
                  std::vector<double>& b) {
  const int N = x.size();
  const int block_size = 512;
  const int num_blocks = N / block_size;

  for ( int r = 0; r < num_blocks; ++r ) {
    for ( int c = 0; c <= r; ++c ) {
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
  }
}

std::vector<double> initForwardSubstitution(std::vector<double>& x, 
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

static void warmupTBB() {
  // This is a simple loop that should get workers started.
  // oneTBB creates workers lazily on first use of the library
  // so this hides the startup time when looking at trivial
  // examples that do little real work. 
  tbb::parallel_for(0, tbb::info::default_concurrency(), 
    [=](int) {
      tbb::tick_count t0 = tbb::tick_count::now();
      while ((tbb::tick_count::now() - t0).seconds() < 0.01);
    }
  );
}

