/*
  Copyright (c) 2018 Intel Corporation

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
   "icpc stereo-serial.cpp ../common/lodepng.cpp -o stereo -ltbb_preview -std=c++11 -I../"

   Simple image processing example to start with
   First task: execute example and note execution time.
*/

#include "../../common/utility/utility.h"

#include "tbb/tbb.h"

#include "../common/utils.h"

using namespace std;

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
            buffer[pixelOffset] = static_cast<uint8_t>(min(buffer[pixelOffset] + channelIncreaseValue, 255));
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

int main() {

    std::cout << "Beginning" << std::endl;

#if _MSC_VER
    std::string inputFileFirst = "../common/input/input1.png";
    std::string inputFileSecond = "../common/input/input2.png";
#else
    std::string inputFileFirst = "../common/input/input1.png";
    std::string inputFileSecond = "../common/input/input2.png";
#endif

    std::string outputFile = "output.png";

    tbb::tick_count mainStartTime = tbb::tick_count::now();

    buffer_t leftImageBuffer = utils::getOrGenerateImage(inputFileFirst);
    buffer_t rightImageBuffer = utils::getOrGenerateImage(inputFileSecond);

    applyImageEffect(leftImageBuffer,redChannelOffset);
    applyImageEffect(rightImageBuffer,blueChannelOffset);

    mergeImageBuffers(leftImageBuffer, rightImageBuffer);

    utils::writePNGImage(rightImageBuffer, outputFile);

    utility::report_elapsed_time((tbb::tick_count::now() - mainStartTime).seconds());

// #if _MSC_VER
//     getchar();
// #endif

    return 0;
}
