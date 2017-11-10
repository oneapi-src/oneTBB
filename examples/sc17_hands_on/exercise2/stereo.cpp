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
        for (int j = 0; j < mat.cols; ++j) {
            Vec3b& bgr = mat.at<Vec3b>(i, j);
            bgr[channelOffset] = saturate_cast<uchar>(bgr[channelOffset] + channelIncreaseValue);
        }
    }, tbb::static_partitioner() );
}

Mat mergeImageBuffers(const Mat& leftImage, const Mat& rightImage) {
    Mat result = leftImage.clone();
    CV_Assert(leftImage.channels() == 3);
    CV_Assert(rightImage.channels() == 3);
    tbb::parallel_for(0,leftImage.rows,[&](int i){
        for (int j = 0; j < leftImage.cols; ++j) {
            Vec3b& bgrResult = result.at<Vec3b>(i, j);
            const Vec3b& bgrLeft = leftImage.at<Vec3b>(i, j);
            const Vec3b& bgrRight = rightImage.at<Vec3b>(i, j);
            bgrResult[2] = bgrRight[2];
        }
    }, tbb::static_partitioner() );
    return result;
}

Mat translateImg(Mat &img, int offsetx, int offsety){
    Mat trans_mat = (Mat_<double>(2,3) << 1, 0, offsetx, 0, 1, offsety);
    warpAffine(img,img,trans_mat,img.size());
    return trans_mat;
}

#if defined(USE_FLOW_GRAPH)

#include "tbb/flow_graph.h"

using namespace tbb::flow;

typedef std::pair<cv::Mat,int> frame_t;

#endif

int main() {
    Mat img;

    tbb::task_scheduler_init init(4);

    VideoCapture cap("../input/input.avi");

    const int limit = 50;
    
    tbb::tick_count mainStartTime;
#if defined(USE_FLOW_GRAPH)
    namedWindow("Flow Graph", WINDOW_AUTOSIZE );
    
    graph g;
    
    int f = 0;
    source_node<frame_t> video_src(g, [&](frame_t &v) -> bool {
        Mat frame;
        if (f < limit) {
            cap >> frame;
            if(!frame.empty()) {
                v = frame_t(frame,f++);
                return true;
            }
        }
        return false;
    }, false);


    // multifunction nodes that prepares greyscale images for left and right eye.
    typedef multifunction_node<frame_t,tuple<frame_t,frame_t>> dispatcher_t;
    dispatcher_t dispatcher(g, unlimited,[](frame_t input, dispatcher_t::output_ports_type& ports) {
        Mat greyMat, img;
        cvtColor(input.first, greyMat, CV_BGR2GRAY);
        cvtColor(greyMat, img, CV_GRAY2BGR);

        Mat leftImage = Mat::zeros(img.size(), img.type());
        Mat rightImage = Mat::zeros(img.size(), img.type());
        img(Rect(10, 0, img.cols - 10, img.rows)).copyTo(leftImage(cv::Rect(0, 0, leftImage.cols - 10, leftImage.rows)));
        img(Rect(0, 0, img.cols - 10, img.rows)).copyTo(rightImage(cv::Rect(10, 0, rightImage.cols - 10, rightImage.rows)));
        get<0>(ports).try_put(frame_t(leftImage, input.second));
        get<1>(ports).try_put(frame_t(rightImage, input.second));
    });

    //function node to apply red filter to left image
    function_node<frame_t, frame_t> left_transform(g,unlimited,[](frame_t in) -> frame_t {
        applyImageEffect(in.first,redChannelOffset);
        return in;
    });

    //function node to apply blue filter to right image
    function_node<frame_t, frame_t> right_transform(g,unlimited,[](frame_t in) -> frame_t {
        applyImageEffect(in.first,blueChannelOffset);
        return in;
    });

#if !defined(TASK_2)
    // Bug 1: Without the tag matching join node nothing is done to ensure correct frames are merged.
    // (Not likely in the given configuration)
    printf("Using Flow Graph version with 2 errors! (see stereo.cpp for details)\n");
    typedef join_node< tuple<frame_t, frame_t>, queueing > join_t;
    join_t image_join(g);
#else
    printf("Using Flow Graph version!\n");
    typedef join_node< tuple<frame_t, frame_t>, tag_matching > join_t;
    join_t image_join(g, [](const frame_t &input) -> int { return input.second; }, [](const frame_t &input) -> int {return input.second;} );
#endif

    function_node<join_t::output_type, frame_t> image_merge(g, unlimited, [&](const join_t::output_type &in) -> frame_t {
        const Mat &left = get<0>(in).first;
        const Mat &right = get<1>(in).first;
        Mat result = mergeImageBuffers(left, right);
        if(get<0>(in).second + 1 == limit) {
            printf("Computation part took:");
            utility::report_elapsed_time((tbb::tick_count::now() - mainStartTime).seconds()); 
        }
        return frame_t(result, get<0>(in).second);
    });

    make_edge(video_src, dispatcher);

    make_edge(output_port<0>(dispatcher), left_transform);
    make_edge(left_transform, input_port<0>(image_join));

    make_edge(output_port<1>(dispatcher), right_transform);
    make_edge(right_transform, input_port<1>(image_join));
    make_edge(image_join, image_merge);

    function_node<frame_t, continue_msg, queueing> image_display(g, serial, [&](frame_t in) -> continue_msg {
        imshow("Flow Graph", in.first);
        waitKey(30);
        return continue_msg();
    });
    sequencer_node<frame_t> sequencer(g, [](const frame_t input){return input.second;});

#if defined(TASK_2)
    // Bug 2: Without the sequencer node frames can overtake each other.
    // (Likely in the given configuration. Visual effect should be obvious :-)
    make_edge(image_merge, sequencer);
    make_edge(sequencer, image_display);
#else
    make_edge(image_merge, image_display);
#endif

#if defined(TBB_PREVIEW_FLOW_GRAPH_TRACE)
    video_src.set_name("Video Source");
    dispatcher.set_name("Dispatcher Node");
    left_transform.set_name("Left Filter");
    right_transform.set_name("Right Filter");
    image_merge.set_name("Merge Image");
    image_join.set_name("Join Images");
    image_display.set_name("Display Image");
#endif

    printf("Beginning'\n'");

    mainStartTime = tbb::tick_count::now();

    video_src.activate();
    g.wait_for_all();

    utility::report_elapsed_time((tbb::tick_count::now() - mainStartTime).seconds());
#else
    namedWindow("Serial", WINDOW_AUTOSIZE );
    
    printf("Using serial version!\n");
    printf("Beginning'\n'");
    mainStartTime = tbb::tick_count::now();
    Mat frame;

    int nframe = 0;
    while(nframe++ < limit) {
        cap >> frame;
        if(frame.empty()) break;
    
        Mat greyMat;
        cvtColor(frame, greyMat, CV_BGR2GRAY);
        cvtColor(greyMat, img, CV_GRAY2BGR);

        Mat leftImage = Mat::zeros(img.size(), img.type());
        Mat rightImage = Mat::zeros(img.size(), img.type());
        img(Rect(10, 0, img.cols - 10, img.rows)).copyTo(leftImage(cv::Rect(0, 0, leftImage.cols - 10, leftImage.rows)));
        img(Rect(0, 0, img.cols - 10, img.rows)).copyTo(rightImage(cv::Rect(10, 0, rightImage.cols - 10, rightImage.rows)));
        
        applyImageEffect(leftImage,2);
        applyImageEffect(rightImage,0);
        
        frame = mergeImageBuffers(leftImage, rightImage);

        if(nframe == limit) {
            printf("Computation part took:");
            utility::report_elapsed_time((tbb::tick_count::now() - mainStartTime).seconds());
        }

        imshow("Serial", frame);
        waitKey(30);
    }
    
    utility::report_elapsed_time((tbb::tick_count::now() - mainStartTime).seconds());
#endif
    waitKey(0);

    return 0;
}

