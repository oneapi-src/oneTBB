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
#include <iostream>
#include <string>

void doWork(double sec);
void taskFunction(const std::string& msg, int id);
static void warmupTBB();

#if TBB_VERSION_MAJOR > 2020
auto P = tbb::info::default_concurrency();

void enqueueSeveralTasks(int num_iterations) {
  tbb::task_arena low_arena{P, 0, tbb::task_arena::priority::low};
  tbb::task_arena normal_arena{P, 0, tbb::task_arena::priority::normal};
  tbb::task_arena high_arena{P, 0, tbb::task_arena::priority::high};

  for (int i = 0; i < num_iterations; ++i) {
    low_arena.enqueue([i]() { taskFunction("L", i); }); 
    normal_arena.enqueue([i]() { taskFunction("N", i); }); 
    high_arena.enqueue([i]() { taskFunction("H", i); }); 
  }
  doWork(1.0);
}
#else
auto P = tbb::task_scheduler_init::default_num_threads();

class MyTask : public tbb::task {
public:

  MyTask(const char *m, int i) : msg(m), messageId(i) { }

  tbb::task *execute() override {
    taskFunction(msg, messageId); 
    return NULL;
  }

private:
  std::string msg;
  int messageId;
};

void enqueueSeveralTasks(int num_iterations) {
  for (int i = 0; i < num_iterations; ++i) {
    tbb::task::enqueue(*new( tbb::task::allocate_root() ) 
                         MyTask( "L", i ), tbb::priority_low);
    tbb::task::enqueue(*new( tbb::task::allocate_root() ) 
                         MyTask( "N", i ), tbb::priority_normal);
    tbb::task::enqueue(*new( tbb::task::allocate_root() ) 
                         MyTask( "H", i ), tbb::priority_high);
  }
  doWork(1.0);
}
#endif

void doWork(double sec) {
  tbb::tick_count t0 = tbb::tick_count::now();
  while ((tbb::tick_count::now() - t0).seconds() < sec);
}

#include <sstream>

tbb::enumerable_thread_specific<std::stringstream> sout;

void printTrace() {
  for (auto& s : sout) {
    std::cout << s.str() << std::endl;
  }
}

void taskFunction(const std::string& msg, int id) {
  sout.local() << msg.c_str() << ":" << id << " ";
  doWork(0.01); 
}

int main() {
  warmupTBB();
  enqueueSeveralTasks(10*P);
  printTrace();
  return 0;
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

