//==============================================================
// Copyright (c) 2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// =============================================================

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include <tbb/tbb.h>


// defined in common/fwd_sub.cpp
std::vector<double> init_fwd_sub(std::vector<double>& x,
                                 std::vector<double>& a,
                                 std::vector<double>& b); 

void check_fwd_sub(const std::vector<double>& x,
                   const std::vector<double>& x_gold); 

using Node = tbb::flow::continue_node<tbb::flow::continue_msg>;
using NodePtr = std::shared_ptr<Node>;

NodePtr create_node(tbb::flow::graph& g, int r, int c, int block_size, 
                   std::vector<double>& x, const std::vector<double>& a, 
                   std::vector<double>& b) {

  const int N = x.size();
  return std::make_shared<Node>(g,
    [r, c, block_size, N, &x, &a, &b] (const tbb::flow::continue_msg& msg) {
      // STEP A: Add node body
      return msg;
    }
  );
}

void add_edges(std::vector<NodePtr>& nodes, int r, int c, int block_size, int num_blocks) {
  NodePtr np = nodes[r*num_blocks + c];
  if (c + 1 < num_blocks && r != c)
    tbb::flow::make_edge(*np, *nodes[r*num_blocks + c + 1]);
  if (r + 1 < num_blocks)
    tbb::flow::make_edge(*np, *nodes[(r + 1)*num_blocks + c]);
}

void fwd_sub_parallel(std::vector<double>& x, const std::vector<double>& a, 
              std::vector<double>& b) {
  const int N = x.size();
  const int block_size = 1024;
  const int num_blocks = N / block_size;

  std::vector<NodePtr> nodes(num_blocks*num_blocks);
  tbb::flow::graph g;
  for (int r = num_blocks - 1; r >= 0; --r) {
    for (int c = r; c >= 0; --c) {
      nodes[r*num_blocks + c] = create_node(g, r, c, block_size, x, a, b);;
      // STEP B: Add a call to function add_edges
    }
  }
  // STEP C: send a tbb::flow::continue_msg{} to nodes[0]
  // STEP D: wait for the graph to complete
}

int main() {
  const int N = 32768;

  std::vector<double> a(N*N);
  std::vector<double> b(N);
  std::vector<double> x(N);

  auto x_gold = init_fwd_sub(x,a,b);

  double parallel_time = 0.0;
  {
    auto pt0 = std::chrono::high_resolution_clock::now();
    fwd_sub_parallel(x,a,b);
    parallel_time = 1e-9*(std::chrono::high_resolution_clock::now() - pt0).count();
  }
  check_fwd_sub(x, x_gold);
  std::cout << "parallel_time == " << parallel_time << " seconds" << std::endl;
  return 0;
}
