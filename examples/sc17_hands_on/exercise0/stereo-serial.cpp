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
    $make

    Simple image processing example to start with
    First task: execute example and note execution time.
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

using namespace cv;
void applyImageEffect(Mat &mat, const int channelOffset) {
    CV_Assert(mat.channels() == 3);
    for (int i = 0; i < mat.rows; ++i) {
        for (int j = 0; j < mat.cols; ++j) {
            Vec3b& bgr = mat.at<Vec3b>(i, j);
            bgr[channelOffset] = saturate_cast<uchar>(bgr[channelOffset] + channelIncreaseValue);
        }
    }
}

void mergeImageBuffers(Mat& leftImage, const Mat& rightImage) {
    CV_Assert(leftImage.channels() == 3);
    CV_Assert(rightImage.channels() == 3);
    for (int i = 0; i < leftImage.rows; ++i) {
        for (int j = 0; j < leftImage.cols; ++j) {
            Vec3b& lbgr = leftImage.at<Vec3b>(i, j);
            const Vec3b& rbgr = rightImage.at<Vec3b>(i, j);
            lbgr[2] = rbgr[2];
        }
    }
}

int main() {

    Mat leftImage, rightImage;

    tbb::tick_count mainStartTime = tbb::tick_count::now();

    try {
        leftImage = imread("../input/input1.png", IMREAD_COLOR);
        rightImage = imread("../input/input2.png", IMREAD_COLOR);
    } catch (Exception& ex) {
        cout << "Error couldn't read Image!" << '\n'; 
        return 1;
    }

    namedWindow("Serial", WINDOW_AUTOSIZE );

    std::cout << "Beginning" << std::endl;

    applyImageEffect(leftImage,2);
    applyImageEffect(rightImage,0);
    mergeImageBuffers(leftImage,rightImage);

    imshow("Serial", leftImage);
    
    utility::report_elapsed_time((tbb::tick_count::now() - mainStartTime).seconds());

    waitKey(0);

    return 0;
}


