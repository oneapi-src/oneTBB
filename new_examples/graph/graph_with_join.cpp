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

#include <iostream>
#include <tbb/tbb.h>

void graphJoin() {
  // step 1: construct the graph
  tbb::flow::graph g;
   
  // step 2: make the nodes
  tbb::flow::function_node my_node{g, 
    tbb::flow::unlimited, 
    []( const int& in ) {
      std::cout << "received: " << in << std::endl; 
      return std::to_string(in);
    }
  };

  tbb::flow::function_node my_other_node{g, 
    tbb::flow::unlimited, 
    [](const int& in) {
      std::cout << "other received: " << in << std::endl; 
      return double(in);
    }
  };

  tbb::flow::join_node<std::tuple<std::string, double>> 
  my_join_node{g};

  tbb::flow::function_node my_final_node{g, 
    tbb::flow::unlimited, 
    [](const std::tuple<std::string, double>& in) {
      std::cout << "final: " << std::get<0>(in) 
                << " and " << std::get<1>(in) << std::endl; 
      return 0;
    }
  };

  // step 3: add the edges
  make_edge(my_node, tbb::flow::input_port<0>(my_join_node));
  make_edge(my_other_node, tbb::flow::input_port<1>(my_join_node));
  make_edge(my_join_node, my_final_node);

  // step 4: send messages
  my_node.try_put(1);
  my_other_node.try_put(2);
  // step 5: wait for the graph to complete
  g.wait_for_all();
}

static void warmupTBB();

int main(int argc, char *argv[]) {
  warmupTBB();
  double parallel_time = 0.0;
  {
    tbb::tick_count t0 = tbb::tick_count::now();
    graphJoin(); 
    parallel_time = (tbb::tick_count::now() - t0).seconds();
  }

  std::cout << "parallel_time == " << parallel_time << " seconds" << std::endl;
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



