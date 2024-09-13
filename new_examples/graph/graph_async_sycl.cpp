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
#include <utility>
#include <sycl/sycl.hpp>
#include <tbb/tbb.h>

int main() {
  const int N = 1000;
  int total_messages = 3;

  using msg_t = std::pair<int, int *>;
  using offload_node_t = tbb::flow::async_node<msg_t, msg_t>;
  using gateway_t = offload_node_t::gateway_type;

  try {
    sycl::queue sycl_q;

    tbb::flow::graph g;

    tbb::flow::input_node<msg_t> 
    in_node{g, 
            [&](tbb::flow_control& fc) {
              int *a = nullptr;
              if (total_messages < 1)
                fc.stop();
              else 
                a = sycl::malloc_shared<int>(N, sycl_q);
              return msg_t{total_messages--, a};
            }
    }; 

    offload_node_t 
    offload_node{g, tbb::flow::unlimited,
                 [&sycl_q](const msg_t& msg, gateway_t& tbb_gateway) {
                   // tell the graph we are doing something asynchronously
                   tbb_gateway.reserve_wait();

                   // submit the async work
                   int id = msg.first;
                   int* a = msg.second;
                   auto sycl_event = sycl_q.fill(a, id, N);

                   // tell the graph the work is done
                   sycl_q.submit([&](sycl::handler& sycl_h) {
                     sycl_h.depends_on(sycl_event); // only run after e is done
                     sycl_h.host_task([&tbb_gateway, msg]() { 
                       // send the result
                       tbb_gateway.try_put(msg);
                       // tell the graph to no longer wait for this agent
                       tbb_gateway.release_wait();
                     });
                   });
                 }
    };

    tbb::flow::function_node<msg_t> 
    out_node{g, tbb::flow::serial,
             [=](const msg_t& msg) {
               int id = msg.first;
               int* a = msg.second;
               if (a == nullptr 
                   || std::any_of(a, a+N, [id](int i) { return i != id; })) {
                 std::cout << "ERROR: unexpected msg in out_node\n"; 
               } else {
                 std::cout << "Received well-formed msg for id " << id << "\n";
               }
             }
    };

    tbb::flow::make_edge(in_node, offload_node);
    tbb::flow::make_edge(offload_node, out_node);
    in_node.activate();
    g.wait_for_all();
  } catch (const sycl::exception&) {
    std::cout << "No CPU SYCL device on this platform\n";
  }
  return 0;
}

