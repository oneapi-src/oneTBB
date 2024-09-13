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

#include <cstdio>
#include <tbb/tbb.h>

void tryPutLoop() {
  const int limit = 3;
  tbb::flow::graph g;
  tbb::flow::function_node my_node{g, tbb::flow::unlimited,
    [](const int& i) { 
      std::printf("my_node: %d\n", i);
      return 0;
    }
  };
  for (int count = 0; count < limit; ++count) {
    int value = count;
    my_node.try_put(value);
  }
  g.wait_for_all();
}

void inputNodeLoop() {
  tbb::flow::graph g;

  tbb::flow::input_node my_input{g, 
    [](tbb::flow_control& fc) { 
      const int limit = 3;
      static int count = 0;
      if (count >= limit) 
        fc.stop();
      return count++;
    }
  };
  tbb::flow::function_node my_node{g, 
    tbb::flow::unlimited,
    [](const int& i) { 
      std::printf("my_node: %d\n", i);
      return 0;
    }
  };

  tbb::flow::make_edge(my_input, my_node);

  my_input.activate();

  g.wait_for_all();
}

static void warmupTBB();

int main() {
  warmupTBB();
  double try_put_time = 0.0, input_node_time = 0.0;
  {
    tbb::tick_count t0 = tbb::tick_count::now();
    tryPutLoop();
    try_put_time = (tbb::tick_count::now() - t0).seconds();
  }
  warmupTBB();
  {
    tbb::tick_count t0 = tbb::tick_count::now();
    inputNodeLoop();
    input_node_time = (tbb::tick_count::now() - t0).seconds();
  }

  std::printf("try_put_time == %f seconds\n", try_put_time);
  std::printf("input_node_time == %f seconds\n", input_node_time);;
  return 0;
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

