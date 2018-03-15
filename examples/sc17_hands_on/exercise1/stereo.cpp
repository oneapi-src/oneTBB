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

/* Instructions:
    Build and run simple image processing example:
    $make

    First task: execute example and note execution time.
    Second task: run Flow Graph version 
        $make CXXFLAGS="-DUSE_FLOW_GRAPH -DSOLUTION" 
        (USE_FLOW_GRAPH and SOLUTION should be defined).
        Execution time should be significantly faster.
    Third task: Inspect applications performance with Intel Flow Graph Analyzer (FGA). 
    
    Optional task: undefine SOLUTION, compare output and look at resulting graph in FGA.*

    *For the given examples traces are collected automatically.
    run:
    $fgt2xml.exe stereo
    to convert collected traces to graphml/traceml format, and open graphml file with FGA
    $fga
    In GUI -> open -> stereo.graphml
 */

#include "../../common/utility/utility.h"

#include "tbb/tbb.h"

using namespace std;

static const int blueChannelOffset = 0;
static const int greenChannelOffset = 1;
static const int redChannelOffset = 2;
static const int channelsPerPixel = 3;
static const int channelIncreaseValue = 10;

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;

#include "tbb/parallel_for.h"

void applyImageEffect(Mat &mat, const int channelOffset) {
    CV_Assert(mat.channels() == 3);
    tbb::parallel_for(0,mat.rows,[&](int i){
    //for (int i = 0; i < mat.rows; ++i) {
        for (int j = 0; j < mat.cols; ++j) {
            Vec3b& bgr = mat.at<Vec3b>(i, j);
            bgr[channelOffset] = saturate_cast<uchar>(bgr[channelOffset] + channelIncreaseValue);
        }
    }, tbb::static_partitioner()
    );
}

Mat mergeImageBuffers(const Mat& leftImage, const Mat& rightImage) {
    Mat result = leftImage.clone();
    CV_Assert(leftImage.channels() == 3);
    CV_Assert(rightImage.channels() == 3);
    tbb::parallel_for(0,leftImage.rows,[&](int i){
//    for (int i = 0; i < leftImage.rows; ++i) {
        for (int j = 0; j < leftImage.cols; ++j) {
            Vec3b& bgrResult = result.at<Vec3b>(i, j);
            //const Vec3b& bgrLeft = leftImage.at<Vec3b>(i, j);
            const Vec3b& bgrRight = rightImage.at<Vec3b>(i, j);
            bgrResult[2] = bgrRight[2];
        }
    }, tbb::static_partitioner()
    );
    return result;
}

#if defined(USE_FLOW_GRAPH)

#include "tbb/flow_graph.h"

using namespace tbb::flow;

#endif

int main() {
    Mat img;

    string file1 = "../input/input1.png";
    string file2 = "../input/input2.png";

    // Set the size of the work stealing scheduler to n=4
    tbb::task_scheduler_init init(4);    
    // Warmup trace collection: 
    tbb::parallel_for(0,tbb::task_scheduler_init::default_num_threads(),[&](int i){
        usleep(3000);
    }, tbb::static_partitioner());

    namedWindow("Test", WINDOW_AUTOSIZE );

#if defined(USE_FLOW_GRAPH)
    graph g;
    
    const int left_limit = 2;
    int l = 1;
    source_node<Mat> left_src(g, [&](Mat &v) -> bool {
        if (l < left_limit) {
            v = imread(file1, IMREAD_COLOR);
            l++;
            return true;
        } else {
            return false;
        }
    }, false);

    const int right_limit = 3;
    int r = 2;
    source_node<Mat> right_src(g, [&](Mat &v) -> bool {
        if (r < right_limit) {
            v = imread(file2, IMREAD_COLOR); 
            r++;
            return true;
        } else {
            return false;
        }
    }, false);

    //function node to apply red filter to left image
    function_node<Mat, Mat> left_transform(g,unlimited,[&](Mat in) -> Mat {
        applyImageEffect(in,redChannelOffset);
        return in;
    });

#if !defined(SOLUTION)
    /* TODO: insert function node to implement blue filter on right image */
#else
    function_node<Mat, Mat> right_transform(g,unlimited,[](Mat in) -> Mat {
        applyImageEffect(in,blueChannelOffset);
        return in;
    });
#endif

    typedef join_node< tuple<Mat, Mat>, queueing > join_t;
    join_t image_join(g);

    function_node<join_t::output_type, Mat> image_merge(g, unlimited, [&](const join_t::output_type &in) -> Mat {
        const Mat &left = get<0>(in);
        const Mat &right = get<1>(in);
        return mergeImageBuffers(left, right);
    });

    function_node<Mat, continue_msg> image_display(g, unlimited, [&](const Mat & in) -> continue_msg {
        img = in;
        imshow("Test", img);
        return continue_msg();
    });

    make_edge(left_src, left_transform);
    make_edge(left_transform, input_port<0>(image_join));

#if !defined(SOLUTION)
    // dummy edge to diplay temporary result
    make_edge(left_transform, image_display);
/*  TODO: make edges.
    right_src -> right_transform);
    right_transform -> input_port<1>(image_join);
    image_join -> image_merge;
*/
#else
    make_edge(right_src, right_transform);
    make_edge(right_transform, input_port<1>(image_join));
    make_edge(image_join, image_merge);
#endif

    make_edge(image_merge, image_display);

#if defined(TBB_PREVIEW_FLOW_GRAPH_TRACE)
    right_src.set_name("Right Source");
    left_src.set_name("Left Source");
    left_transform.set_name("Left Filter");
#if defined(SOLUTION)
    right_transform.set_name("Right Filter");
#endif
    image_merge.set_name("Merge Image");
    image_display.set_name("Display Image");
#endif

    std::cout << "Beginning" << std::endl;

    tbb::tick_count mainStartTime = tbb::tick_count::now();

    try {
        left_src.activate();
        right_src.activate();
        g.wait_for_all();
    } catch (Exception& ex) {
        cout << "Error couldn't read Image!" << '\n';
        return 1;
    }

    utility::report_elapsed_time((tbb::tick_count::now() - mainStartTime).seconds());
#else
    tbb::tick_count mainStartTime = tbb::tick_count::now();
    Mat leftImageBuffer, rightImageBuffer;

    try {
        leftImageBuffer = imread(file1, IMREAD_COLOR);
        rightImageBuffer = imread(file2, IMREAD_COLOR);
    } catch (Exception& ex) {
        cout << "Error couldn't read Image!" << '\n';
        return 1;
    }        

    applyImageEffect(leftImageBuffer,2);
    applyImageEffect(rightImageBuffer,0);
    
    img = mergeImageBuffers(leftImageBuffer, rightImageBuffer);

    if(!img.empty()) {
        imshow("Test", img);
    }
    utility::report_elapsed_time((tbb::tick_count::now() - mainStartTime).seconds());
#endif
    waitKey(0);

    return 0;
}

