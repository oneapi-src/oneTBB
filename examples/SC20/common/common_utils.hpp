//==============================================================
// Copyright (c) 2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// =============================================================

#pragma once

#include <exception>
#include <iostream>
#include <numeric>
#include <stdlib.h>

#include <CL/sycl.hpp>

namespace common {

// this exception handler with catch async exceptions
static auto exception_handler = [](cl::sycl::exception_list eList) {
  for (std::exception_ptr const &e : eList) {
    try {
      std::rethrow_exception(e);
    } catch (std::exception const &e) {
#if _DEBUG
      std::cout << "Failure" << std::endl;
#endif
      std::terminate();
    }
  }
};

template<typename ArrayType>
void print_array(const char* text, const ArrayType& array) {
    std::cout << text;
  for (const auto& s : array) std::cout << s << ' ';
  std::cout << "\n";
}

void print_array(const char* text, const float * array, int array_size) {
    std::cout << text;
  for (int i=0; i<array_size; ++i) std::cout << array[i] << ' ';
  std::cout << "\n";
}

template<typename ArrayType>
void init_input_arrays(ArrayType &a_array, ArrayType &b_array) {
  std::iota(a_array.begin(), a_array.end(), 0);
  std::iota(b_array.begin(), b_array.end(), 0);
}

template<typename ArrayType>
void validate_results(float alpha, const ArrayType& a_array, const ArrayType& b_array,
		      const ArrayType& c_sycl, const ArrayType& c_tbb) {
  // Create gold standard / serial result
  ArrayType c_gold;

  for (size_t i = 0; i < a_array.size(); ++i)
    c_gold[i] = a_array[i] + alpha * b_array[i];

  // Compare golden triad with heterogeneous triad
  if (!std::equal(std::begin(c_sycl), std::end(c_sycl), std::begin(c_gold)))
    std::cout << "SYCL triad error.\n";
  else
    std::cout << "SYCL triad correct.\n";

  // Compare golden triad with TBB triad
  if (!std::equal(std::begin(c_tbb), end(c_tbb),
                  std::begin(c_gold)))
    std::cout << "TBB triad error.\n";
  else
    std::cout << "TBB triad correct.\n";
}
    
template<typename ArrayType>
void validate_hetero_results(float ratio, float alpha_sycl, float alpha_tbb, const ArrayType& a_array, 
                             const ArrayType& b_array, const ArrayType& c_array) {

  std::size_t ratio_switch_pt = static_cast<std::size_t>(std::ceil(c_array.size() * ratio));
    
  // Create gold standard / serial result
  ArrayType c_gold;
  for (size_t i = 0; i < ratio_switch_pt; ++i)
    c_gold[i] = a_array[i] + alpha_sycl * b_array[i];
  for (size_t i = ratio_switch_pt; i < c_array.size(); ++i)
    c_gold[i] = a_array[i] + alpha_tbb * b_array[i];
    
  // Compare golden triad with heterogeneous triad
  if (!std::equal(std::begin(c_array), std::end(c_array), std::begin(c_gold)))
    std::cout << "Hetero triad error.\n";
  else
    std::cout << "Hetero triad correct.\n";
}

void validate_usm_results(float ratio, float alpha_sycl, float alpha_tbb, const float* a_array, 
                             const float* b_array, const float* c_array, int array_size) {

  std::size_t ratio_switch_pt = static_cast<std::size_t>(std::ceil(array_size * ratio));
    
  // Create gold standard / serial result
  float* c_gold=new float[array_size];
  for (size_t i = 0; i < ratio_switch_pt; ++i)
    c_gold[i] = a_array[i] + alpha_sycl * b_array[i];
  for (size_t i = ratio_switch_pt; i < array_size; ++i)
    c_gold[i] = a_array[i] + alpha_tbb * b_array[i];
    
  // Compare golden triad with heterogeneous triad
  if (!std::equal(c_array, c_array+array_size, c_gold))
    std::cout << "Hetero triad error.\n";
  else
    std::cout << "Hetero triad correct.\n";
  delete[] c_gold;
}
    

template<typename ArrayType>
void print_results(float alpha, const ArrayType& a_array, const ArrayType& b_array,
                   const ArrayType& c_sycl, const ArrayType& c_tbb) {
  print_array("input array a_array: ", a_array);
  print_array("input array b_array: ", b_array);
  std::cout << "alpha: " << alpha << "\n";
  print_array("output array c using SYCL on GPU: ", c_sycl);
  print_array("output array c using TBB on CPU: ", c_tbb);
}
    
template<typename ArrayType>
void print_hetero_results(float ratio, float alpha_sycl, float alpha_tbb, const ArrayType& a_array, 
                          const ArrayType& b_array, const ArrayType& c_array) {
  print_array("input array a_array: ", a_array);
  print_array("input array b_array: ", b_array);
  std::cout << "ratio: " << ratio << "\n";
  std::cout << "switch_index: " << static_cast<std::size_t>(std::ceil(c_array.size() * ratio)) << "\n";
  std::cout << "alpha_sycl: " << alpha_sycl << "\n";
  std::cout << "alpha_tbb: " << alpha_tbb << "\n";
  print_array("output array c_array: ", c_array);
}

void print_usm_results(float ratio, float alpha_sycl, float alpha_tbb, const float *a_array, 
                          const float *b_array, const float *c_array, int array_size) {
  print_array("input array a_array: ", a_array, array_size);
  print_array("input array b_array: ", b_array, array_size);
  std::cout << "ratio: " << ratio << "\n";
  std::cout << "switch_index: " << static_cast<std::size_t>(std::ceil(array_size * ratio)) << "\n";
  std::cout << "alpha_sycl: " << alpha_sycl << "\n";
  std::cout << "alpha_tbb: " << alpha_tbb << "\n";
  print_array("output array c_tbb: ", c_array, array_size);
}
    
};  // namespace common

