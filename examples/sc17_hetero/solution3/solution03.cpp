/*
    Copyright (c) 2017 Intel Corporation

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

/* Build instructions:
 Linux:   icpc solution03.cpp -o hello_cl -ltbb_preview -std=c++11 -lOpenCL
 MacOS:   icpc solution03.cpp -o hello_cl -ltbb_preview -std=c++11 -framework OpenCL
 */

#define TBB_PREVIEW_FLOW_GRAPH_NODES 1
#define TBB_PREVIEW_FLOW_GRAPH_FEATURES 1

#include "tbb/flow_graph.h"
#include "tbb/flow_graph_opencl_node.h"

using namespace tbb;
using namespace tbb::flow;

#include "tbb/flow_graph.h"
#include <cstdio>

class gpu_device_selector {
public:
    template <typename DeviceFilter>
    tbb::flow::opencl_device operator()(tbb::flow::opencl_factory<DeviceFilter>& f) {
        // Set your GPU device if available to execute kernel on
        const tbb::flow::opencl_device_list &devices = f.devices();
        tbb::flow::opencl_device_list::const_iterator it = std::find_if(
            devices.cbegin(), devices.cend(),
            [](const tbb::flow::opencl_device &d) {
            cl_device_type type;
            d.info(CL_DEVICE_TYPE, type);
            return CL_DEVICE_TYPE_GPU == type;
        });

        if (it == devices.cend()) {
            std::cout << "Info: could not find any GPU devices. Choosing the first available device (default behaviour)." << std::endl;
            return *(f.devices().begin());
        } else {
            // Return GPU device from factory
            return *it;
        }
    }
};

int main() {
    typedef opencl_buffer<cl_char> buffer_t;
    typedef int token_t;
    
    graph g;
    gpu_device_selector gpu_selector;

    opencl_program<> program("hello_world.cl");
    opencl_node<tuple<buffer_t>> gpu_node(g, program.get_kernel("print"));
    std::array<unsigned int, 1> range{1};
    gpu_node.set_range(range);

    buffer_node<token_t> token_buffer(g);
    
    typedef multifunction_node< token_t, tuple< buffer_t, buffer_t > > mfn_t;
    mfn_t dispatch_node(g, unlimited, [&]( const token_t &in, mfn_t::output_ports_type &ports ) {
        token_t tok = in;
        const char str[] = "Hello from ";
        buffer_t b( sizeof(str) );
        std::copy_n( str, sizeof(str), b.begin() );
        printf("Dispatcher received %d\n", tok);
        if (tok == 0) {
            get<0>(ports).try_put(b);
        } else {
            get<1>(ports).try_put(b);
        }
    });
    
    function_node<buffer_t, token_t> cpu_node(g,unlimited,[](buffer_t b) {
        char *str = (char*)b.begin();
        for ( ; *str; ++str ) printf("%c", *str);
        printf("CPU!\n");
        return token_t(0);
    });
    
    make_edge(token_buffer,dispatch_node);
    make_edge(output_port<0>(dispatch_node), cpu_node);
    make_edge(output_port<1>(dispatch_node), gpu_node);

    printf("Beginning\n");
    
    token_buffer.try_put(token_t(0));
    token_buffer.try_put(token_t(1));
    
    g.wait_for_all();
    
#if _MSC_VER
	getchar();
#endif
    return 0;
}
