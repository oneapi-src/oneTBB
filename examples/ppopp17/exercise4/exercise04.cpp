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

/*
 
 Task: compile, run and compare resulting runtime to previous version of stereo.
 
 Build instructions:
 Linux:   icpc exercise04.cpp ../common/lodepng.cpp -o stereo_cl -ltbb_preview -std=c++11 -lOpenCL
 MacOS:   icpc exercise04.cpp ../common/lodepng.cpp -o stereo_cl -ltbb_preview -std=c++11 -framework OpenCL
 
 */

#include "../../common/utility/utility.h"

/* Begin: Duplicate preamble from previous stereo exercise

#include "tbb/tbb.h"

typedef utils::image_buffer buffer_t;

static const int redChannelOffset = 0;
static const int greenChannelOffset = 1;
static const int blueChannelOffset = 2;
static const int channelsPerPixel = 4;
static const int channelIncreaseValue = 10;

 End: Duplicate from previous exercise */

#define TBB_PREVIEW_FLOW_GRAPH_NODES 1
#define TBB_PREVIEW_FLOW_GRAPH_FEATURES 1

#include "tbb/flow_graph.h"
#include "tbb/flow_graph_opencl_node.h"
#include "tbb/tick_count.h"

#include "../common/utils.h"

using namespace tbb;
using namespace tbb::flow;

typedef opencl_buffer<cl_uchar> OpenclImageBuffer;
typedef std::array<unsigned int, 2> NDRange;
typedef tuple< OpenclImageBuffer, cl_uint, NDRange > buffer_t;
typedef tuple< OpenclImageBuffer, OpenclImageBuffer, cl_uint, NDRange > merge_buffer_t;

void fillOpenclBuffer(tbb::flow::opencl_buffer<cl_uchar>& openclBuffer, const std::vector<unsigned char>& sourceBuffer) {
    std::copy(sourceBuffer.begin(), sourceBuffer.end(), openclBuffer.begin());
}

class stereo_node : public composite_node< tuple<buffer_t, buffer_t>, tuple<buffer_t> > {
    typedef composite_node< tuple<buffer_t, buffer_t>, tuple<buffer_t> > stereo_base_type;
    opencl_graph &my_graph;
    opencl_program<> program;
    split_node<buffer_t> left_split;
    split_node<buffer_t> right_split;

    opencl_node< buffer_t > left_transform;
    opencl_node< buffer_t > right_transform;

    opencl_node< merge_buffer_t > image_merge;
    join_node< buffer_t > image_join;
    
public:
    stereo_node(opencl_graph &g) : my_graph(g), stereo_base_type(g),
    program(g, "stereo.cl"),
    left_transform(g, program.get_kernel("applyLeftImageEffect")),
    right_transform(g, program.get_kernel("applyRightImageEffect")),
    image_merge(g, program.get_kernel("mergeImages")),
    image_join(g),
    right_split(g),
    left_split(g) {
        make_edge(output_port<0>(left_split), input_port<0>(left_transform));
        make_edge(output_port<1>(left_split), input_port<1>(left_transform));
        make_edge(output_port<2>(left_split), input_port<2>(left_transform));
        make_edge(output_port<0>(right_split), input_port<0>(right_transform));
        make_edge(output_port<1>(right_split), input_port<1>(right_transform));
        make_edge(output_port<2>(right_split), input_port<2>(right_transform));
        
        make_edge(output_port<0>(left_transform), input_port<0>(image_merge));
        make_edge(output_port<0>(right_transform), input_port<1>(image_merge));
        make_edge(output_port<1>(left_transform), input_port<2>(image_merge));
        make_edge(output_port<2>(left_transform), input_port<3>(image_merge));

        make_edge(output_port<0>(image_merge), input_port<0>(image_join));
        make_edge(output_port<2>(image_merge), input_port<1>(image_join));
        make_edge(output_port<3>(image_merge), input_port<2>(image_join));
        
        left_transform.set_args(port_ref<0, 1>());
        left_transform.set_range(port_ref<2>());
        
        right_transform.set_args(port_ref<0, 1>());
        right_transform.set_range(port_ref<2>());
        
        image_merge.set_args(port_ref<0, 2>());
        image_merge.set_range(port_ref<3>());

        set_external_ports(input_ports_type(left_split, right_split), output_ports_type(image_join));
    }
};

void chooseTargetDevice(tbb::flow::opencl_graph& g) {
    // Set your GPU device if available to execute kernel on
    const opencl_device_list &devices = g.available_devices();
    opencl_device_list::const_iterator it = std::find_if(devices.cbegin(), devices.cend(), [](const opencl_device &d) {
        cl_device_type type;
        d.info(CL_DEVICE_TYPE, type);
        return CL_DEVICE_TYPE_GPU == type;
    });
    
    if (it == devices.cend()) {
        std::cout << "Info: could not find any GPU devices. Choosing the first available device (default behaviour)." << std::endl;
    } else {
        // Init factory with GPU device
        g.opencl_factory().init({ *it });
    }
}

int main() {
    opencl_graph g;
    chooseTargetDevice(g);
    
    std::string inputFileFirst = "../common/input/input1.png";
    std::string inputFileSecond = "../common/input/input2.png";
    std::string outputFile = "output.png";
    
    const int left_limit = 2;
    int l = 1;
    source_node<buffer_t> left_src(g, [&](buffer_t &v) -> bool {
        if (l < left_limit) {
            utils::image_buffer src = utils::getOrGenerateImage(inputFileFirst);
            // Create and initialize opencl_buffer in order to pass it to mergeImages kernel
            OpenclImageBuffer oclImage(g, src.buffer->size());
            fillOpenclBuffer(oclImage, *src.buffer);
            NDRange rangeList = { src.width, src.height };
            v = std::make_tuple(oclImage, src.width, rangeList);
            l++;
            return true;
        } else {
            return false;
        }
    }, false);
    
    const int right_limit = 3;
    int r = 2;
    source_node<buffer_t> right_src(g, [&](buffer_t &v) -> bool {
        if (r < right_limit) {
            utils::image_buffer src = utils::getOrGenerateImage(inputFileSecond);
            // Create and initialize opencl_buffer in order to pass it to mergeImages kernel
            OpenclImageBuffer oclImage(g, src.buffer->size());
            fillOpenclBuffer(oclImage, *src.buffer);
            NDRange rangeList = { src.width, src.height };
            v = std::make_tuple(oclImage, src.width, rangeList);
            r++;
            return true;
        } else {
            return false;
        }
    }, false);
    
    stereo_node stereo(g);

    function_node<buffer_t, buffer_t> image_display(g, unlimited, [&](buffer_t b) {
        // The result image have to be copied in order to be changed,
        // the second parameter - image size, can be taken by const reference
        OpenclImageBuffer imageBuffer = std::get<0>(b);
        const NDRange& imageSize = std::get<2>(b);
        unsigned int width = imageSize[0];
        unsigned int height = imageSize[1];
        
        utils::writePNGImage(imageBuffer.data(), width, height, outputFile);
        return b;
    });
    
    make_edge(left_src, input_port<0>(stereo));
    make_edge(right_src, input_port<1>(stereo));
    make_edge(stereo, image_display);
    
    std::cout << "Beginning" << std::endl;
    
    tbb::tick_count mainStartTime = tbb::tick_count::now();
    
    left_src.activate();
    right_src.activate();
    
    g.wait_for_all();
    
    utility::report_elapsed_time((tbb::tick_count::now() - mainStartTime).seconds());
    
    return 0;
}


