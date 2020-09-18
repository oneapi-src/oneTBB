//==============================================================
// Copyright (c) 2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// =============================================================

#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <tbb/tbb.h>

#define INCORRECT_VALUE 1
#define INCORRECT_MODE_A tbb::filter_mode::serial_in_order
#define INCORRECT_MODE_B tbb::filter_mode::parallel

using CaseStringPtr = std::shared_ptr<std::string>;

//
// These functions are defined in common/case.cpp
//
void initCaseChange(int num_strings, int string_len, int free_list_size);
CaseStringPtr getCaseString(std::ofstream& f); 
void writeCaseString(std::ofstream& f, CaseStringPtr s);

void change_case_parallel(int num_tokens, std::ofstream& caseBeforeFile, std::ofstream& caseAfterFile) {
  tbb::parallel_pipeline(
    /* STEP A: set the number of tokens */ INCORRECT_VALUE,
    tbb::make_filter<void, CaseStringPtr>(
      tbb::filter_mode::serial_in_order,
      [&](tbb::flow_control& fc) -> CaseStringPtr {
        CaseStringPtr s_ptr = getCaseString(caseBeforeFile);
        if (!s_ptr) 
          fc.stop();
        return s_ptr; 
      }) &
    tbb::make_filter<CaseStringPtr, CaseStringPtr>(
      /* STEP B: Change the mode to be parallel */ INCORRECT_MODE_A,
      [](CaseStringPtr s_ptr) -> CaseStringPtr {
        /* STEP C: add the body that converts the incoming s_ptr */
        return s_ptr;
      }) & // concatenation operation
    tbb::make_filter<CaseStringPtr, void>(
      /* STEP D: Change the mode to serial_in_order */ INCORRECT_MODE_B,
      [&](CaseStringPtr s_ptr) -> void {
        /* STEP E: add the body to write the output */
      }) 
  );
}

static void warmupTBB() {
  int num_threads = std::thread::hardware_concurrency();
  tbb::parallel_for(0, num_threads,
    [](unsigned int) { 
      std::this_thread::sleep_for(std::chrono::milliseconds(10)); 
  });
}

int main() {
  int num_tokens = std::thread::hardware_concurrency();
  int num_strings = 100; 
  int string_len = 100000;
  int free_list_size = num_tokens;

  std::ofstream caseBeforeFile("lab/parallel_pipeline_before.txt");
  std::ofstream caseAfterFile("lab/parallel_pipeline_after.txt");
  initCaseChange(num_strings, string_len, free_list_size);

  warmupTBB();
  double parallel_time = 0.0;
  {
    auto pt0 = std::chrono::high_resolution_clock::now();
    change_case_parallel(num_tokens, caseBeforeFile, caseAfterFile);
    parallel_time = 1e-9*(std::chrono::high_resolution_clock::now() - pt0).count();
  }
  std::cout << "parallel_time == " << parallel_time << " seconds" << std::endl;
  return 0;
}
