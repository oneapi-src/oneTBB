//==============================================================
// Copyright (c) 2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// =============================================================

#include <iomanip>
#include <iostream>

#include <CL/sycl.hpp>

#include <chrono>

#include <oneapi/dpl/iterator>
#include <oneapi/dpl/execution>
#include <oneapi/dpl/algorithm>

#include "utils.hpp"

using namespace cl::sycl;
using namespace oneapi::dpl::execution;

int main() {
  // Image size is width x height
  int width = 720;
  int height = 480;

  Img<ImgFormat::BMP> image{width, height};
  ImgFractal fractal{width, height};

  // Lambda to process image with gamma = 0.5
  auto gamma_f = [](ImgPixel& pixel) {
    float v = (0.3f * pixel.r + 0.59f * pixel.g + 0.11f * pixel.b) / 255.0f;

    auto gamma_pixel = static_cast<std::uint8_t>(255.0f * v * v);
    if (gamma_pixel > 255) gamma_pixel = 255;
    pixel.set(gamma_pixel, gamma_pixel, gamma_pixel, gamma_pixel);
  };

  // fill image with created fractal
  int index = 0;
  image.fill([&index, width, &fractal](ImgPixel& pixel) {
    int x = index % width;
    int y = index / width;

    auto fractal_pixel = fractal(x, y);
    if (fractal_pixel < 0) fractal_pixel = 0;
    if (fractal_pixel > 255) fractal_pixel = 255;
    pixel.set(fractal_pixel, fractal_pixel, fractal_pixel, fractal_pixel);

    ++index;
  });

  Img<ImgFormat::BMP> image2 = image;
  image.write("fractal_original.png");

  // call standard serial function for correctness check
    image.fill(gamma_f);  
    image.write("fractal_gamma.png");

  // create a queue for tasks, sent to the device
  //  Select either the gpu_selector or the cpu_selector or the default_selector
  //queue q(gpu_selector{});
  //queue q(cpu_selector{});
  queue q(default_selector{});

  // We need a new scope to control the buffer's destruction and copy back the data
  {
    // ****Step 1: Creating a SYCL buffer for moving data on target device
    buffer<ImgPixel, 1> buffer(image2.data(),image2.width() * image2.height());

    // ****Step 2: Create buffer iterators. These are passed to the algorithm
    auto buffer_begin = oneapi::dpl::begin(buffer);
    auto buffer_end = oneapi::dpl::end(buffer);

    //*****Step 3: Create a new device policy
    auto new_policy = make_device_policy(q);
    //*****Step 4: Call std::for_each with DPC++ support    
    std::for_each(new_policy, buffer_begin, buffer_end, gamma_f);   
  }

  // check correctness
  if (check(image.begin(), image.end(), image2.begin())) {
    std::cout << "success";
  } else {
    std::cout << "fail";
  }
  std::cout << ". Run on "
            << q.get_device().get_info<cl::sycl::info::device::name>()
            << std::endl;

  image2.write("fractal_gamma_pstl_with_sycl.png");

  return 0;
}
