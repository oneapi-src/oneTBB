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

#include "../../common/utility/utility.h"

#define TBB_PREVIEW_FLOW_GRAPH_NODES 1
#define TBB_PREVIEW_FLOW_GRAPH_FEATURES 1

#include "tbb/flow_graph.h"
#include "tbb/flow_graph_opencl_node.h"
#include "tbb/tick_count.h"

#include "../common/utils.h"

using namespace tbb;
using namespace tbb::flow;

typedef std::array<unsigned int, 2> NDRange;
typedef tuple< cl_uint, cl_uint, cl_uint > fractal_data_t;
typedef tuple< opencl_buffer<cl_uchar>, cl_uint, cl_uint, cl_uint, cl_uint > fractal_cpu_data_t;
typedef tuple< opencl_buffer<cl_uchar>, cl_uint, cl_uint, cl_uint, cl_uint, NDRange > fractal_gpu_data_t;
typedef tuple< opencl_buffer<cl_uchar>, cl_uint, NDRange > buffer_t;

typedef tuple< opencl_buffer<cl_uchar>, opencl_buffer<cl_uchar>, cl_uint, NDRange > merge_buffer_t;

static const int image_size = 1024;
static const int max_iterations = 256;

static const int divider = 2;
static const int tile_width = image_size/divider;
static const int tile_height = image_size/divider;

static const int channelsPerPixel = 4;

class color_fractal_cpu_node : public composite_node< tuple<fractal_data_t>, tuple<buffer_t> > {
    typedef composite_node< tuple<fractal_data_t>, tuple<buffer_t> > fractal_cpu_base_type;
    opencl_graph &my_graph;
    function_node<fractal_data_t, buffer_t> calculate_fractal;
    
    void calculate_pixel(unsigned char * pixel_ptr, int pixelOffset, int x0, int y0, int max_iterations, int size_x,int size_y, float magn, float cx, float cy) {
        unsigned int iter;
        double fx0, fy0, xtemp, x, y, mu;
        
        fx0 = (double)x0 - (double) size_x / 2.0;
        fy0 = (double)y0 - (double) size_y / 2.0;
        fx0 = fx0 / magn + cx;
        fy0 = fy0 / magn + cy;
        
        iter = 0; x = 0; y = 0;
        mu = 0;
        
        while (((x*x + y*y) <= 32) && (iter < max_iterations)) {
            xtemp = x*x - y*y + fx0;
            y = 2*x*y + fy0;
            x = xtemp;
            mu += exp(-sqrt(x*x+y*y));
            iter++;
        }
        
        if (iter == max_iterations) {
            // point corresponds to the mandelbrot set
            pixel_ptr[pixelOffset] = pixel_ptr[pixelOffset+1] = pixel_ptr[pixelOffset+2] = pixel_ptr[pixelOffset+3] = 255;
            return;
        }
        
        int b = (int)(256*mu);
        int g = (b/8);
        int r = (g/16);
        
        pixel_ptr[pixelOffset] = r>255 ? 255 : r;
        pixel_ptr[pixelOffset+1] = g>255 ? 255 : g;
        pixel_ptr[pixelOffset+2] = b>255 ? 255 : b;
        pixel_ptr[pixelOffset+3] = 255;
    }
    
public:
    color_fractal_cpu_node(opencl_graph &g) : my_graph(g), fractal_cpu_base_type(g),
    calculate_fractal( my_graph, unlimited, [&](fractal_data_t data) -> buffer_t {
        printf("color_fractal_cpu_node received %d\n", get<0>(data));
        opencl_buffer<cl_uchar> oclImage(g, tile_width*tile_height*channelsPerPixel);
        NDRange rangeList = { tile_width, tile_height };
        const int y0 = get<1>(data);
        const int x0 = get<0>(data);
        for(int y = y0; y < y0 + tile_height; ++y) {
            for(int x = x0; x < x0 + tile_width; ++x) {
                const int pixelOffset = ((y-y0) * tile_width + (x-x0)) * channelsPerPixel;
                calculate_pixel(oclImage.data(),pixelOffset,x,y,max_iterations,image_size,image_size, 400.0f, -.6f, .6f);
            }
        }
        
        return std::make_tuple(oclImage, get<2>(data), rangeList);
    }) {
        set_external_ports(input_ports_type(calculate_fractal), output_ports_type(calculate_fractal));
    }
    
};

class mono_fractal_gpu_node : public composite_node< tuple<fractal_gpu_data_t>, tuple<buffer_t> > {
    typedef composite_node< tuple<fractal_gpu_data_t>, tuple<buffer_t> > fractal_gpu_base_type;
    opencl_graph &my_graph;
    opencl_program<> program;
    
    split_node<fractal_gpu_data_t> fractal_data_split;
    opencl_node<fractal_gpu_data_t> fractal_compute_node;
    join_node<buffer_t> image_join;

public:
    mono_fractal_gpu_node(opencl_graph &g, float shift) : my_graph(g), fractal_gpu_base_type(g),
    program(g, "fractal.cl"),
    fractal_compute_node(g, program.get_kernel("mono_mandelbrot")),
    image_join(g),
    fractal_data_split(g) {
        make_edge(output_port<0>(fractal_data_split), input_port<0>(fractal_compute_node));
        make_edge(output_port<1>(fractal_data_split), input_port<1>(fractal_compute_node));
        make_edge(output_port<2>(fractal_data_split), input_port<2>(fractal_compute_node));
        make_edge(output_port<3>(fractal_data_split), input_port<3>(fractal_compute_node));
        make_edge(output_port<4>(fractal_data_split), input_port<4>(fractal_compute_node));
        make_edge(output_port<5>(fractal_data_split), input_port<5>(fractal_compute_node));
        fractal_compute_node.set_args(port_ref<0>(), port_ref<1>(), port_ref<2>(), port_ref<3>(), port_ref<4>(), max_iterations, image_size, image_size, 400.0f, -.6f + shift, .6f);
        fractal_compute_node.set_range(port_ref<5>());
        make_edge(output_port<0>(fractal_compute_node), input_port<0>(image_join));
        make_edge(output_port<4>(fractal_compute_node), input_port<1>(image_join));
        make_edge(output_port<5>(fractal_compute_node), input_port<2>(image_join));
        set_external_ports(input_ports_type(fractal_data_split), output_ports_type(image_join));
    }
};

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
        
        left_transform.set_args(port_ref<0>(), tile_width);
        left_transform.set_range(port_ref<2>());
        
        right_transform.set_args(port_ref<0>(), tile_width);
        right_transform.set_range(port_ref<2>());
        
        image_merge.set_args(port_ref<0, 1>(), tile_width);
        image_merge.set_range(port_ref<3>());
        
        set_external_ports(input_ports_type(left_split, right_split), output_ports_type(image_join));
    }
};

void chooseTargetDevice(tbb::flow::opencl_graph& g) {
    // Set your GPU device if available to execute kernel on
    const tbb::flow::opencl_device_list &devices = g.available_devices();
    tbb::flow::opencl_device_list::const_iterator it = std::find_if(devices.cbegin(), devices.cend(), [](const tbb::flow::opencl_device &d) {
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
    
    color_fractal_cpu_node render_fractal_cpu(g);
    mono_fractal_gpu_node render_left_fractal(g, -0.05f);
    mono_fractal_gpu_node render_right_fractal(g, 0.05f);
    stereo_node stereo(g);
    
    std::string outputFile = "output";
    
    source_node<fractal_data_t> tileSource(g, [&](fractal_data_t &v) -> bool {
        static int tag = 0;
        if ( tag < 4 ) {
            int x0 = 0, y0 = 0;
            
            if (tag == 1 || tag == 3) x0 = 512;
            if (tag == 2 || tag == 3) y0 = 512;
            
            v = std::make_tuple(x0, y0, tag);
            std::cout << "Hello from source node " << x0 << " ," << y0 << " ," << tag << std::endl;
            tag++;
            return true;
        }
        else {
            return false;
        }
    }, false);

    typedef multifunction_node< fractal_data_t, tuple< fractal_gpu_data_t, fractal_gpu_data_t, cl_uint > > stereo_mfn_t;
    stereo_mfn_t fractalInput(g, unlimited, [&]( const stereo_mfn_t::input_type &in, stereo_mfn_t::output_ports_type &ports ) {
        opencl_buffer<cl_uchar> oclImage1(g, tile_width*tile_height*channelsPerPixel);
        opencl_buffer<cl_uchar> oclImage2(g, tile_width*tile_height*channelsPerPixel);
        NDRange rangeList = { tile_width, tile_height };
        /* render left Image */
        get<0>(ports).try_put(std::make_tuple(oclImage1, get<0>(in), get<0>(in) + tile_width, get<1>(in), get<1>(in) + tile_height, rangeList ));
        /* redner right Image */
        get<1>(ports).try_put(std::make_tuple(oclImage2, get<0>(in), get<0>(in) + tile_width, get<1>(in), get<1>(in) + tile_height, rangeList ));
        /* forward tile-tag to token buffer */
        get<2>(ports).try_put(get<2>(in));
    });
 
    typedef cl_uint token_t;
    typedef join_node< tuple<fractal_data_t, token_t>, reserving > rj_t;
    rj_t rj(g);
    
    typedef multifunction_node< rj_t::output_type, tuple< fractal_data_t, fractal_data_t > > mfn_t;
    mfn_t dispatch_node(g, unlimited, []( const mfn_t::input_type &in, mfn_t::output_ports_type &ports ) {
        fractal_data_t d = get<0>(in);
        token_t tok = get<1>(in);
        printf("Dispatcher received %d\n", tok);
        if (tok == 0) {
            get<0>(ports).try_put(d);
        } else {
            get<1>(ports).try_put(d);
        }
    });
    
    // generate token
    buffer_node<token_t> token_buffer(g);
    
    make_edge(tileSource, input_port<0>(rj));
    make_edge(token_buffer, input_port<1>(rj));
    make_edge(rj, dispatch_node);
    make_edge(output_port<0>(dispatch_node), render_fractal_cpu);
    make_edge(output_port<1>(dispatch_node), fractalInput);
    
    static utils::image_buffer timeline;
    static utils::image_buffer image;

    image.width = image_size;
    image.height = image_size;
    image.buffer = std::make_shared< std::vector<unsigned char> >(image.width * image.height * 4);
    timeline.width = 4 * tile_width;
    timeline.height = tile_height;
    image.buffer = std::make_shared< std::vector<unsigned char> >(timeline.width * timeline.height * 4);
    
    function_node<buffer_t, buffer_t> image_display(g, serial, [&](buffer_t b) {
        static int frame = 0;
        int tag = std::get<1>(b);

        opencl_buffer<cl_uchar> tileBuffer = std::get<0>(b);
        const NDRange& imageSize = std::get<2>(b);
        unsigned int tile_width = imageSize[0];
        unsigned int tile_height = imageSize[1];
        
        std::vector<unsigned char>& imageBuffer = *image.buffer;
        std::vector<unsigned char>& chronoBuffer = *timeline.buffer;
 
        int x0 = 0, y0 = 0;
        
        if (tag == 1 || tag == 3) x0 = 512;
        if (tag == 2 || tag == 3) y0 = 512;
        
        for (unsigned y = y0; y < y0 + tile_height; y++) {
            /* over all rows */
            for (unsigned x = x0; x < x0 + tile_width; x++) {
                /* over all columns */
                for (unsigned i = 0; i < 4; ++i) {
                    /* over all channels */
                    imageBuffer.data()[4 * (image.width * y + x) + i] = tileBuffer.data()[4 * (tile_width * (y - y0) + (x - x0)) + i];
                }
            }
        }
        
        printf("Hello from image node %d, %d, %d \n", x0, y0, tag);
        utils::writePNGImage(imageBuffer.data(), image.width, image.height, outputFile + std::to_string(frame) + ".png");
        frame++;
        return b;
    });
    
    typedef join_node< tuple<buffer_t, cl_uint>, queueing > rj_token_t;
    
    rj_token_t rj_gpu_token(g);

    typedef multifunction_node< rj_token_t::output_type, tuple< buffer_t, token_t > > mfn_gpu_token_t;
    
    mfn_gpu_token_t gpu_token_recycle(g, unlimited, [](const mfn_gpu_token_t::input_type &in, mfn_gpu_token_t::output_ports_type &ports) {
        const buffer_t& b = get<0>(in);
        const cl_uint& tag = get<1>(in);
        printf("Hello from token GPU recycler!");
        get<0>(ports).try_put(std::make_tuple( get<0>(b) ,tag , get<2>(b) ));
        get<1>(ports).try_put(token_t(1));
    });
    
    make_edge( stereo, input_port<0>(rj_gpu_token)); // in: buffer_t
    make_edge( output_port<2>(fractalInput), input_port<1>(rj_gpu_token)); // in: token_t
    make_edge( rj_gpu_token, gpu_token_recycle); // in: buffer_t + token_t
    
    make_edge( output_port<0>(gpu_token_recycle), image_display );
    make_edge( output_port<1>(gpu_token_recycle), token_buffer );
    
    make_edge( output_port<0>(fractalInput), render_left_fractal);
    make_edge( output_port<1>(fractalInput), render_right_fractal);
    
    make_edge( output_port<0>(render_left_fractal), input_port<0>(stereo));
    make_edge( output_port<0>(render_right_fractal), input_port<1>(stereo));
    
    rj_token_t rj_cpu_token(g);
    
    typedef multifunction_node< buffer_t, tuple< buffer_t, token_t > > mfn_cpu_token_t;
    mfn_cpu_token_t cpu_token_recycle(g, unlimited, [](const mfn_cpu_token_t::input_type &in, mfn_cpu_token_t::output_ports_type &ports) {
        const cl_uint& tag = get<1>(in);
        printf("Hello from token CPU recycler!");
        get<0>(ports).try_put(in);
        get<1>(ports).try_put(token_t(0));
    });
    
    make_edge( render_fractal_cpu, cpu_token_recycle );
    
    make_edge( output_port<0>(cpu_token_recycle), image_display );
    make_edge( output_port<1>(cpu_token_recycle), token_buffer );
    
    printf("Beginning\n");
    tbb::tick_count mainStartTime = tbb::tick_count::now();
    token_buffer.try_put(token_t(0));
    token_buffer.try_put(token_t(1));
    tileSource.activate();
    g.wait_for_all();
    utility::report_elapsed_time((tbb::tick_count::now() - mainStartTime).seconds());
    return 0;
}

