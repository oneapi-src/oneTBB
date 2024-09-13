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
#include <memory>
#include <tbb/tbb.h>

struct config_t { 
  int id; 
  int predecessor; 
};  

config_t configuration[] = { { 0, 3 }, 
                           { 1, 4 }, 
                           { 2, 5 }, 
                           { 3, -1 }, 
                           { 4, -1 }, 
                           { 5, 3 }, 
                           { 6, 4 }, 
                           { 7, 1 } };

int num_nodes = sizeof(configuration) / sizeof(config_t);

int main() {
  tbb::flow::graph g;

  // create a vector of pointers to work nodes
  using work_node_t = tbb::flow::continue_node<tbb::flow::continue_msg>;
  std::vector<std::unique_ptr<work_node_t>> work_nodes(num_nodes); 

  // create fully populated vector of "promises"
  using future_node_t = tbb::flow::write_once_node<tbb::flow::continue_msg>;
  std::vector<future_node_t> future_nodes(num_nodes, future_node_t{g}); 

  // build graph (starting nodes with no predecessors)
  for(int i = 0; i < num_nodes; ++i) {
    const config_t& c = configuration[i];

    // create the work node for that element
    work_nodes[c.id].reset(
        new work_node_t{g, 
                        [c](const tbb::flow::continue_msg& m) {
                          std::printf("Executing %d\n", c.id); 
                          return m;
                        }
                       });
    // connect the new node to its future
    tbb::flow::make_edge(*work_nodes[c.id], future_nodes[c.id]); 

    // start the node or link it to predecessor's promise
    if (c.predecessor != -1) {
       std::printf("new %d with %d -> %d\n", c.id, c.predecessor, c.id);
       tbb::flow::make_edge(future_nodes[c.predecessor], *work_nodes[c.id]);
    } else {
       std::printf("starting %d from main\n", c.id);
       work_nodes[c.id]->try_put(tbb::flow::continue_msg{});
    }
    std::printf("**** wait a bit in main\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::printf("**** done waiting in main\n");
  }

  g.wait_for_all();
  return 0;
}

