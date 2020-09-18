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

using CaseStringPtr = std::shared_ptr<std::string>;

//
// These functions are defined in common/case.cpp
//
void initCaseChange(int num_strings, int string_len, int free_list_size);
CaseStringPtr getCaseString(std::ofstream& f); 
void writeCaseString(std::ofstream& f, CaseStringPtr s);
    
void change_case_serial(std::ofstream& caseBeforeFile, std::ofstream& caseAfterFile) {
  while (CaseStringPtr s_ptr = getCaseString(caseBeforeFile)) {
    std::transform(s_ptr->begin(), s_ptr->end(), s_ptr->begin(), 
      [](char c) -> char {
        if (std::islower(c))
          return std::toupper(c);
        else if (std::isupper(c))
          return std::tolower(c);
        else
          return c;
      }
    );
    writeCaseString(caseAfterFile, s_ptr);
  }
}

int main() {
  int num_strings = 100; 
  int string_len = 100000;
  int free_list_size = 1;

  std::ofstream caseBeforeFile("lab/serial_pipeline_before.txt");
  std::ofstream caseAfterFile("lab/serial_pipeline_after.txt");
  initCaseChange(num_strings, string_len, free_list_size);

  double serial_time = 0.0;
  {
    auto pt0 = std::chrono::high_resolution_clock::now();
    change_case_serial(caseBeforeFile, caseAfterFile);
    serial_time = 1e-9*(std::chrono::high_resolution_clock::now() - pt0).count();
  }
  std::cout << "serial_time == " << serial_time << " seconds" << std::endl;
  return 0;
}
