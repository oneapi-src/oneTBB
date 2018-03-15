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
 
 Your task is to complete the composite node, which encapsulates the functionality from previous exercises.
 
 Build instructions:
 icpc exercise02.cpp ../common/lodepng.cpp -o stereo -ltbb_preview -std=c++11 -I../
 
 Begin: Duplicate preamble from previous exercise */

#include "../../common/utility/utility.h"

#include "tbb/tbb.h"

#include "../common/utils.h"

typedef utils::image_buffer buffer_t;

static const int redChannelOffset = 0;
static const int greenChannelOffset = 1;
static const int blueChannelOffset = 2;
static const int channelsPerPixel = 4;
static const int channelIncreaseValue = 10;

void applyImageEffect(buffer_t& image, const int channelOffset) {
    const int heighBase = channelsPerPixel * image.width;
    std::vector<unsigned char>& buffer = *image.buffer;
    
    // Increase selected color channel by a predefined value
    for (unsigned int y = 0; y < image.height; y++) {
        const int heightOffset = heighBase * y;
        for (unsigned int x = 0; x < image.width; x++) {
            int pixelOffset = heightOffset + channelsPerPixel * x + channelOffset;
            buffer[pixelOffset] = static_cast<uint8_t>(std::min(buffer[pixelOffset] + channelIncreaseValue, 255));
        }
    }
}

// This function merges to image buffers into the first buffer (leftImageBuffer as a destination)
void mergeImageBuffers(buffer_t& leftImage, const buffer_t& rightImage) {
    const int heighBase = channelsPerPixel * leftImage.width;
    std::vector<unsigned char>& leftImageBuffer = *leftImage.buffer;
    std::vector<unsigned char>& rightImageBuffer = *rightImage.buffer;
    
    // Apply stereoscopic merge using algorithm: R: left image, G:, B: right image
    for (unsigned int y = 0; y < leftImage.height; y++) {
        const int heightOffset = heighBase * y;
        for (unsigned int x = 0; x < leftImage.width; x++) {
            const int pixelOffset = heightOffset + channelsPerPixel * x;
            const int redChannelIndex = pixelOffset + redChannelOffset;
            rightImageBuffer[redChannelIndex] = leftImageBuffer[redChannelIndex];
        }
    }
}

/* End: Duplicate from previous exercise */

#include "tbb/flow_graph.h"

using namespace tbb;
using namespace tbb::flow;

class stereo_node : public composite_node< tuple<buffer_t, buffer_t>, tuple<buffer_t> > {
    typedef composite_node< tuple<buffer_t, buffer_t>, tuple<buffer_t> > base_type;
    graph &my_graph;
    function_node<buffer_t, buffer_t> left_transform;
    function_node<buffer_t, buffer_t> right_transform;
    typedef join_node< tuple<buffer_t, buffer_t>, queueing > join_t;
    join_t image_join;
    function_node<join_t::output_type, buffer_t> image_merge;
    
public:
    stereo_node(graph &g) : my_graph(g), base_type(g),
    left_transform(g,unlimited,[](buffer_t b) {
        applyImageEffect(b,redChannelOffset);
        return b;
    }),
    right_transform(g,unlimited,[](buffer_t b) {
/* TODO: apply right image effect */
        return b;
    }),
    image_join(g),
    image_merge(g, unlimited,
                [](const join_t::output_type &in) -> buffer_t {
/* TODO: merge left and right image. Hint: see last exercise */
                    buffer_t right = get<1>(in);

                    return right;
                }) {
                    make_edge(left_transform, input_port<0>(image_join));
                    make_edge(right_transform, input_port<1>(image_join));
                    make_edge(image_join, image_merge);
                    set_external_ports(input_ports_type(left_transform, right_transform), output_ports_type(image_merge));
                }
};

int main() {
    graph g;
    
    std::string inputFileFirst = "../common/input/input1.png";
    std::string inputFileSecond = "../common/input/input2.png";
    std::string outputFile = "output.png";
    
    const int left_limit = 2;
    int l = 1;
    source_node<buffer_t> left_src(g, [&](buffer_t &v) -> bool {
        if (l < left_limit) {
            v = utils::getOrGenerateImage(inputFileFirst);
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
            v = utils::getOrGenerateImage(inputFileSecond);
            r++;
            return true;
        } else {
            return false;
        }
    }, false);
    
    stereo_node stereo(g);

    function_node<buffer_t, buffer_t> image_display(g, unlimited, [&](buffer_t b) {
        utils::writePNGImage(b, outputFile);
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


