//==============================================================
// Copyright (c) 2020 Intel Corporation
//
// SPDX-License-Identifier: Apache 2.0
// =============================================================

#include "../common/common_utils.hpp"

#include <cmath>  //for std::ceil
#include <array>
#include <atomic>
#include <iostream>
#include <thread>

#include <CL/sycl.hpp>

#include <tbb/blocked_range.h>
#include <tbb/flow_graph.h>
#include <tbb/global_control.h>
#include <tbb/parallel_for.h>

constexpr size_t array_size = 16;

template<size_t ARRAY_SIZE>
struct msg_t {
  static constexpr size_t array_size = ARRAY_SIZE;
  const float offload_ratio = 0.5;
  const float alpha_0 = 0.5;
  const float alpha_1 = 1.0;
  std::array<float, array_size> a_array;  // input
  std::array<float, array_size> b_array;  // input
  std::array<float, array_size> c_sycl;   // GPU output
  std::array<float, array_size> c_tbb;    // CPU output
};

using msg_ptr = std::shared_ptr<msg_t<array_size>>;

using async_node_t = tbb::flow::async_node<msg_ptr, msg_ptr>;
using gateway_t = async_node_t::gateway_type;

class AsyncActivity {
  msg_ptr msg;
  gateway_t* gateway_ptr;
  std::atomic<bool> submit_flag;
  std::thread service_thread;

 public:
  AsyncActivity() : msg{nullptr}, gateway_ptr{nullptr}, submit_flag{false},
    service_thread( [this] {
      //Wait until other thread sets submit_flag=true
      while( !submit_flag ) std::this_thread::yield();
      // Here we go! Dispatch code to the GPU
      // Execute the kernel over a portion of the array range
      size_t array_size_sycl = std::ceil(msg->a_array.size() * msg->offload_ratio);
      {  
        sycl::buffer a_buffer{msg->a_array}, b_buffer{msg->b_array}, c_buffer{msg->c_sycl};
        sycl::queue q{sycl::gpu_selector{}};
        float alpha = msg->alpha_0;
        q.submit([&, alpha](sycl::handler& h) {            
          sycl::accessor a_accessor{a_buffer, h, sycl::read_only};
          sycl::accessor b_accessor{b_buffer, h, sycl::read_only};
          sycl::accessor c_accessor{c_buffer, h, sycl::write_only};
          h.parallel_for(sycl::range<1>{array_size_sycl}, [=](sycl::id<1> index) {
            c_accessor[index] = a_accessor[index] + b_accessor[index] * alpha;
          });  
        }).wait();
      }
      gateway_ptr->try_put(msg);
      gateway_ptr->release_wait();
    } ) {}

  ~AsyncActivity() {
    service_thread.join();
  }

  void submit(msg_ptr m, gateway_t& gateway) {
    gateway.reserve_wait();
    msg = m;
    gateway_ptr = &gateway;
    submit_flag = true;
  }
};

int main() {
  tbb::flow::graph g;

  // Input node:
  tbb::flow::input_node<msg_ptr> in_node{g, 
    [&](tbb::flow_control& fc) -> msg_ptr {
      static bool has_run = false;
      if (has_run) fc.stop();
      has_run = true; // This example only creates a message to feed the Flow Graph
      msg_ptr msg = std::make_shared<msg_t<array_size>>();
      common::init_input_arrays(msg->a_array, msg->b_array);
      return msg;
    }
  };

  // CPU node
  tbb::flow::function_node<msg_ptr, msg_ptr> cpu_node{
      g, tbb::flow::unlimited, [&](msg_ptr msg) -> msg_ptr {
        size_t i_start = static_cast<size_t>(std::ceil(msg->array_size * msg->offload_ratio));
        size_t i_end = static_cast<size_t>(msg->array_size);
        auto &a_array = msg->a_array, &b_array = msg->b_array, &c_tbb = msg->c_tbb;
        float alpha = msg->alpha_1;
        tbb::parallel_for(tbb::blocked_range<size_t>{i_start, i_end},
          [&, alpha](const tbb::blocked_range<size_t>& r) {
            for (size_t i = r.begin(); i < r.end(); ++i)
              c_tbb[i] = a_array[i] + alpha * b_array[i];
            }
        );
        return msg;
      }};

  // async node -- GPU
  AsyncActivity async_act;
  async_node_t a_node{g, tbb::flow::unlimited,
    [&async_act](msg_ptr msg, gateway_t& gateway) {
      async_act.submit(msg, gateway);
    }
  };

  // join node
  using join_t = tbb::flow::join_node<std::tuple<msg_ptr, msg_ptr>>;
  join_t node_join{g};

  // out node
  tbb::flow::function_node<join_t::output_type> out_node{g, tbb::flow::unlimited, 
    [&](const join_t::output_type& two_msgs) {
      msg_ptr msg = std::get<0>(two_msgs); //Both msg's point to the same data
      //Merge GPU result into CPU array
      std::size_t gpu_end = static_cast<std::size_t>(std::ceil(msg->array_size * msg->offload_ratio));
      std::copy(msg->c_sycl.begin(), msg->c_sycl.begin()+gpu_end, msg->c_tbb.begin());
      common::validate_hetero_results(msg->offload_ratio, msg->alpha_0, msg->alpha_1, 
                                      msg->a_array, msg->b_array, msg->c_tbb);
      if(msg->array_size<=64)
        common::print_hetero_results(msg->offload_ratio, msg->alpha_0, msg->alpha_1, 
                                     msg->a_array, msg->b_array, msg->c_tbb);
    }
  };  // end of out node

  // construct graph
  tbb::flow::make_edge(in_node, a_node);
  tbb::flow::make_edge(in_node, cpu_node);
  tbb::flow::make_edge(a_node, tbb::flow::input_port<0>(node_join));
  tbb::flow::make_edge(cpu_node, tbb::flow::input_port<1>(node_join));
  tbb::flow::make_edge(node_join, out_node);

  in_node.activate();
  g.wait_for_all();

  return 0;
}
