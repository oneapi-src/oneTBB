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

#include <tbb/tbb.h>

void doWork(double seconds);

#if TBB_VERSION_MAJOR > 2020
const int N = 2*tbb::info::default_concurrency();

void setThreadsAndSlots() {
  tbb::global_control gc(tbb::global_control::max_allowed_parallelism, N);
  tbb::task_arena a{N};

  a.execute([]() {
    tbb::parallel_for(0, 
                      10*N, 
                      [](int) { doWork(0.01); });
  });
}
#else
#warning Using tbb::task_scheduler_init instead of tbb::global_control
const int N = tbb::task_scheduler_init::default_num_threads();

void setThreadsAndSlots() {
  tbb::task_scheduler_init init(N);

  tbb::parallel_for(0, 
                    10*N, 
                    [](int) { doWork(0.01); });
}
#endif

#include <atomic>
#include <cstdio>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <vector>

std::atomic<int> next_tid;
tbb::enumerable_thread_specific<int> my_tid(-1);
std::vector<std::atomic<int>> tid_participation(2*N);

void noteParticipation() {
  auto& t = my_tid.local();
  if (t == -1) {
    t = next_tid++;
  }
  ++tid_participation[t];
}

void doWork(double seconds) {
  noteParticipation();
  tbb::tick_count t0 = tbb::tick_count::now();
  while ((tbb::tick_count::now() - t0).seconds() < seconds);
}

void clearParticipation() {
  next_tid = 0;
  my_tid.clear();
  for (auto& p : tid_participation)
    p = 0;
}

void dumpParticipation(int p) {
  int sum = tid_participation[0];
  std::cout << "[" << tid_participation[0];
  for (int i = 1; i < p; ++i) {
    sum += tid_participation[i];
    std::cout << ", " << tid_participation[i];
  }
  for (int i = p; i < 2*N; ++i) 
    std::cout << ", -";
  std::cout << "]\n" 
            << "sum == " << sum  << "\n"
            << "expected sum " << 2*10*N << "\n";
  clearParticipation();
}

int main() {
  setThreadsAndSlots();
  dumpParticipation(2*N);
  return 0;
}

