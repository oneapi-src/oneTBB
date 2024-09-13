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
const int P = 2*tbb::info::default_concurrency();

void taskBlocking() {
  tbb::task_group g;
  g.run([]() { doWork(0.1); }); 
  g.run([]() { doWork(0.2); }); 
  g.wait();
}
#else
const int P = tbb::task_scheduler_init::default_num_threads();

class MyTask : public tbb::task {
  double my_time;
public:
  MyTask(double t=0.0) : my_time(t) {}
  virtual tbb::task* execute() {
    doWork(my_time);
    return nullptr;
  }
};

void taskBlocking() {
  MyTask& root = *new(tbb::task::allocate_root()) MyTask{};

  MyTask& child1 = 
    *new(root.allocate_child()) MyTask{0.1};
  MyTask& child2 = 
    *new(root.allocate_child()) MyTask{0.2};

  root.set_ref_count(3);

  tbb::task::spawn(child1);
  tbb::task::spawn(child2);
  root.wait_for_all();
}
#endif

void parallelInvoke() {
  tbb::parallel_invoke([]() { doWork(0.1); },
                       []() { doWork(0.2); } );
}

static   void warmupTBB();

int main() {
  warmupTBB();
  std::printf("taskBlocking:\n");
  taskBlocking();
  std::printf("parallelInvoke:\n");
  parallelInvoke();
  return 0;
}

void doWork(double seconds) {
  std::printf("doing work for %f seconds\n", seconds);
  tbb::tick_count t0 = tbb::tick_count::now();
  while ((tbb::tick_count::now() - t0).seconds() < seconds);
}

static void warmupTBB() {
  // This is a simple loop that should get workers started.
  // oneTBB creates workers lazily on first use of the library
  // so this hides the startup time when looking at trivial
  // examples that do little real work. 
  tbb::parallel_for(0, P, 
   [=](int) {
    tbb::tick_count t0 = tbb::tick_count::now();
    while ((tbb::tick_count::now() - t0).seconds() < 0.01);
  }
  );
}


