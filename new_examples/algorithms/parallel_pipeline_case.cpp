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

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <tbb/tbb.h>

using CaseStringPtr = std::shared_ptr<std::string>;
CaseStringPtr getCaseString(std::ofstream& f); 
void writeCaseString(std::ofstream& f, CaseStringPtr s);

void serialChangeCase(std::ofstream& caseBeforeFile, 
                      std::ofstream& caseAfterFile) {
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

void parallelChangeCase(std::ofstream& caseBeforeFile, 
                        std::ofstream& caseAfterFile) {
  int num_tokens = tbb::info::default_concurrency();
  tbb::parallel_pipeline(
    /* tokens */ num_tokens,
    /* the get filter */
    tbb::make_filter<void, CaseStringPtr>(
      /* filter node */ tbb::filter_mode::serial_in_order,
      /* filter body */
      [&](tbb::flow_control& fc) -> CaseStringPtr {
        CaseStringPtr s_ptr = getCaseString(caseBeforeFile);
        if (!s_ptr) 
          fc.stop();
        return s_ptr; 
      }) & // concatenation operation
    /* make the change case filter */
    tbb::make_filter<CaseStringPtr, CaseStringPtr>(
      /* filter node */ tbb::filter_mode::parallel,
      /* filter body */
      [](CaseStringPtr s_ptr) -> CaseStringPtr {
        std::transform(s_ptr->begin(), s_ptr->end(), s_ptr->begin(), 
          [](char c) -> char {
            if (std::islower(c))
              return std::toupper(c);
            else if (std::isupper(c))
              return std::tolower(c);
            else
              return c;
          });
        return s_ptr;
      }) & // concatenation operation
    /* make the write filter */
    tbb::make_filter<CaseStringPtr, void>(
      /* filter node */ tbb::filter_mode::serial_in_order,
      /* filter body */
      [&](CaseStringPtr s_ptr) -> void {
        writeCaseString(caseAfterFile, s_ptr);
      }) 
  );
}


void initCaseChange(int num_strings, int string_len, int free_list_size);
static void warmupTBB();

int main() {
  int num_strings = 100; 
  int string_len = 100000;

  double serial_time = 0.0, tbb_time = 0.0;
  {
    std::ofstream caseBeforeFile("change_case_before.txt");
    std::ofstream caseAfterFile("change_case_serial_after.txt");
    initCaseChange(num_strings, string_len, tbb::info::default_concurrency());
    tbb::tick_count t0 = tbb::tick_count::now();
    serialChangeCase(caseBeforeFile, caseAfterFile);
    serial_time = (tbb::tick_count::now() - t0).seconds();
  }
  warmupTBB();
  {
    std::ofstream caseBeforeFile("change_case_before.txt");
    std::ofstream caseAfterFile("change_case_parallel_after.txt");
    initCaseChange(num_strings, string_len, tbb::info::default_concurrency());
    tbb::tick_count t0 = tbb::tick_count::now();
    parallelChangeCase(caseBeforeFile, caseAfterFile);
    tbb_time = (tbb::tick_count::now() - t0).seconds();
  }
  std::cout << "serial_time == " << serial_time << " seconds" << std::endl;
  std::cout << "tbb_time == " << tbb_time << " seconds" << std::endl;
  std::cout << "speedup (serial_time/tbb_time) == " << serial_time/tbb_time << std::endl;
  return 0;
}

static tbb::concurrent_queue<CaseStringPtr> caseFreeList;
static int numCaseInputs = 0;

void initCaseChange(int num_strings, int string_len, int free_list_size) {
  numCaseInputs = num_strings;
  caseFreeList.clear();
  for (int i = 0; i < free_list_size; ++i) {
    caseFreeList.push(std::make_shared<std::string>(string_len, ' '));
  }
}

CaseStringPtr getCaseString(std::ofstream& f) {
  std::shared_ptr<std::string> s;
  if (numCaseInputs > 0) {
    if (!caseFreeList.try_pop(s) || !s) {
      std::cerr << "Error: Ran out of elements in free list!" << std::endl;
      return CaseStringPtr{};
    }
    int ascii_range = 'z' - 'A' + 2;
    for (int i = 0; i < s->size(); ++i) {
      int offset = i%ascii_range;
      if (offset)
        (*s)[i] = 'A' + offset - 1; 
      else
        (*s)[i] = '\n';
    } 
    if (f.good()) {
      f << *s;
    }
    --numCaseInputs;
  }
  return s;
}

void writeCaseString(std::ofstream& f, CaseStringPtr s) {
  if (f.good()) {
    f << *s;
  }
  caseFreeList.push(s);
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



