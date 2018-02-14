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

#include <iostream>
#include <cstdlib>
#include <vector>

#include <tbb/tbb.h>
#include <tbb/flow_graph.h>


template<typename T> void nrand(T& a)
{
  static const double scale = 1.0 / RAND_MAX;
  a = static_cast<T>(scale * (2 * std::rand() - RAND_MAX));
}


template<typename T>
void dot(T& result, const T* a, const T* b, std::size_t size)
{
  result = 0;
  for (std::size_t i = 0; i < size; ++i) {
    result += a[i] * b[i];
  }
}


template<typename T>
void gemv(T* result, const T* matrix, const T* vector, std::size_t nrows, std::size_t ncols)
{
  for (std::size_t i = 0; i < nrows; ++i) {
    dot(result[i], matrix + i * ncols, vector, ncols);
  }
}


template<typename T>
class gemv_body {
public:
  gemv_body(T* result, const T* matrix, std::size_t nrows, std::size_t ncols)
    : res(result), mat(matrix), m(nrows), n(ncols)
  {}

  const T* operator()(const T* vector) const {
    gemv(res, mat, vector, m, n);
    return res;
  }

private:
  T* res;
  const T* mat;
  std::size_t m, n;
};


template<typename T>
class dot_body {
public:
  dot_body(T& result, const T* vector, std::size_t size)
    : r(&result), t(vector), n(size)
  {}

  const T operator()(const T* vector) const {
    dot(*r, t, vector, n);
    return *r;
  }

private:
  T* r;
  const T* t;
  std::size_t n;
};


int main(int argc, char* argv[])
{
  try {
    using namespace tbb;
    typedef float T;
    typedef std::vector<T> B; // buffer

    std::size_t nrows = 1 < argc ? std::atol(argv[1]) : 512;
    std::size_t ncols = 2 < argc ? std::atol(argv[2]) : nrows;
    std::size_t nrept = 5;

    B m[] = { B(nrows * ncols), B(nrows * ncols) };
    std::size_t matrices = sizeof(m) / sizeof(*m);
    for (std::size_t i = 0; i < matrices; ++i) {
      std::for_each(m[i].begin(), m[i].end(), nrand<T>);
    }

    B v[] = { B(ncols), B(ncols), B(ncols) };
    std::size_t vectors = sizeof(v) / sizeof(*v);
    for (std::size_t i = 0; i < vectors; ++i) {
      std::for_each(v[i].begin(), v[i].end(), nrand<T>);
    }

    // temporary/intermediate result vectors
    B t[] = { B(nrows), B(nrows), B(nrows), B(nrows), B(nrows), B(nrows) };
    T r[4];

    std::cout << "Expression Evaluation (serial)\n";
    double min_time_serial = std::numeric_limits<double>::max();
    double min_time_short = std::numeric_limits<double>::max();
    for (std::size_t i = 0; i < nrept; ++i) {
      tick_count start = tick_count::now();
      gemv(&t[0][0], &m[0][0], &v[0][0], nrows, ncols);
      gemv(&t[1][0], &m[1][0], &t[0][0], nrows, ncols);
      dot(r[0], &v[2][0], &t[1][0], std::min(nrows, ncols));
      tick_count t1 = tick_count::now();
      gemv(&t[2][0], &m[0][0], &v[1][0], nrows, ncols);
      gemv(&t[3][0], &m[1][0], &t[2][0], nrows, ncols);
      dot(r[1], &v[2][0], &t[3][0], std::min(nrows, ncols));
      tick_count t2 = tick_count::now();
      gemv(&t[4][0], &m[1][0], &v[0][0], nrows, ncols);
      dot(r[2], &v[2][0], &t[4][0], std::min(nrows, ncols));
      gemv(&t[5][0], &m[1][0], &v[1][0], nrows, ncols);
      dot(r[3], &v[2][0], &t[5][0], std::min(nrows, ncols));
      tick_count end = tick_count::now();
      const double time = (end - start).seconds();
      min_time_serial = std::min(min_time_serial, time);
      std::cout << " Time " << (i + 1) << ": "<< (1000 * time) << " ms\n";
      min_time_short = std::min(min_time_short, 0.5 * ((end - t1) + (end - t2)).seconds());
    }
    std::cout << "Minimum: " << (1000 * min_time_serial) << " ms\n";
    std::cout << "Results: ";
    std::copy(r, r + sizeof(r) / sizeof(*r), std::ostream_iterator<T>(std::cout, " "));
    std::cout << "\n\n";

    flow::graph g;

    typedef flow::function_node<const T*, const T*> gemv_node_t;
    typedef flow::function_node<const T*, T> dot_node_t;

    gemv_node_t g1(g, flow::unlimited, gemv_body<T>(&t[0][0], &m[0][0], nrows, ncols));
    gemv_node_t g2(g, flow::unlimited, gemv_body<T>(&t[1][0], &m[1][0], nrows, ncols));
    dot_node_t d1(g, flow::unlimited, dot_body<T>(r[0], &v[2][0], std::min(nrows, ncols)));

    gemv_node_t g3(g, flow::unlimited, gemv_body<T>(&t[2][0], &m[0][0], nrows, ncols));
    gemv_node_t g4(g, flow::unlimited, gemv_body<T>(&t[3][0], &m[1][0], nrows, ncols));
    dot_node_t d2(g, flow::unlimited, dot_body<T>(r[1], &v[2][0], std::min(nrows, ncols)));

    gemv_node_t g5(g, flow::unlimited, gemv_body<T>(&t[4][0], &m[1][0], nrows, ncols));
    dot_node_t d3(g, flow::unlimited, dot_body<T>(r[2], &v[2][0], std::min(nrows, ncols)));

    gemv_node_t g6(g, flow::unlimited, gemv_body<T>(&t[5][0], &m[1][0], nrows, ncols));
    dot_node_t d4(g, flow::unlimited, dot_body<T>(r[3], &v[2][0], std::min(nrows, ncols)));

    flow::make_edge(g1, g2); flow::make_edge(g2, d1);
    flow::make_edge(g3, g4); flow::make_edge(g4, d2);
    flow::make_edge(g5, d3);
    flow::make_edge(g6, d4);

    std::cout << "Expression Evaluation (flow::graph)\n";
    double min_time_graph = std::numeric_limits<double>::max();
    for (std::size_t i = 0; i < nrept; ++i) {
      tick_count start = tick_count::now();
      g1.try_put(&v[0][0]);
      g3.try_put(&v[1][0]);
      g5.try_put(&v[0][0]);
      g6.try_put(&v[1][0]);
      g.wait_for_all();
      tick_count end = tick_count::now();
      const double time = (end - start).seconds();
      min_time_graph = std::min(min_time_graph, time);
      std::cout << " Time " << (i + 1) << ": "<< (1000 * time) << " ms\n";
    }
    std::cout << "Minimum: " << (1000 * min_time_graph) << " ms\n";
    std::cout << "Results: ";
    std::copy(r, r + sizeof(r) / sizeof(*r), std::ostream_iterator<T>(std::cout, " "));
    std::cout << "\n\n";

    std::cout << "Comparison\n";
    std::cout << "Short Path: " << (1000 * min_time_short) << " ms\n";
    std::cout << "Graph Time: " << (1000 * min_time_graph) << " ms Speedup: ";
    std::cout << min_time_short / min_time_graph << "x\n";
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
