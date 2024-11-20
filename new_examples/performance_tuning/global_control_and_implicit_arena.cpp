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

const int default_P = tbb::info::default_concurrency();

void noteParticipation(); /* record info for participation vector */
void dumpParticipation(int p); /* display participation vector */

void doWork(double seconds) {
  noteParticipation();
  tbb::tick_count t0 = tbb::tick_count::now();
  while ((tbb::tick_count::now() - t0).seconds() < seconds);
}

void arenaGlobalControlImplicitArena(int p) {
  tbb::global_control gc(tbb::global_control::max_allowed_parallelism, p);
  tbb::parallel_for(0, 
                    10*tbb::info::default_concurrency(), 
                    [](int) { doWork(0.01); });
}


int main() {
  arenaGlobalControlImplicitArena(default_P);
  dumpParticipation(default_P);
  arenaGlobalControlImplicitArena(default_P/2);
  dumpParticipation(default_P/2);
  arenaGlobalControlImplicitArena(2*default_P);
  dumpParticipation(2*default_P);
  return 0;
}

#include <atomic>
#include <iostream>
#include <vector>

std::atomic<int> next_tid;
tbb::enumerable_thread_specific<int> my_tid(-1);
std::vector<std::atomic<int>> tid_participation(2*default_P);

void noteParticipation() {
  auto& t = my_tid.local();
  if (t == -1) {
    t = next_tid++;
  }
  ++tid_participation[t];
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
  for (int i = 1; i < std::min(p, default_P); ++i) {
    sum += tid_participation[i];
    std::cout << ", " << tid_participation[i];
  }
  for (int i = p; i < default_P; ++i) 
    std::cout << ", -";
  std::cout << "]\n" 
            << "sum == " << sum  << "\n"
            << "expected sum " << 10*default_P << "\n\n";
  clearParticipation();
}



