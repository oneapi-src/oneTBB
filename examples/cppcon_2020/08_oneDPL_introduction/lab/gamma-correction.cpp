//==============================================================
// Copyright (c) 2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// =============================================================

#include <iomanip>
#include <iostream>

#include <chrono>

#include <oneapi/dpl/execution>
#include <oneapi/dpl/algorithm>

#include "utils.hpp"

int main() {
  // Image size is width x height
  const int width = 450;
  const int height = 450;

  Img<ImgFormat::BMP> image{width, height};
  ImgFractal fractal{width, height};

  // Lambda to process pixel with gamma correction
  auto gamma_f = [](ImgPixel& pixel) {
    /* compute correction factor */
    const double gamma = 0.7;
    double gamma_correction = 1.0 / gamma;
    /* convert to graysale */
    double v = (0.3 * pixel.r + 0.59 * pixel.g + 0.11 * pixel.b) / 255.0 ;
    /* apply gamma correction */
    double res = 255.0 * pow(v, gamma_correction);
    /* saturated cast for out of bound check */
    auto gamma_pixel = static_cast<std::uint8_t>(res);
    if (gamma_pixel > UINT8_MAX) gamma_pixel = UINT8_MAX;
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

  // Image processing with Parallel STL
  std::for_each(std::execution::par, image2.begin(), image2.end(), gamma_f);

  // check correctness
  if (check(image.begin(), image.end(), image2.begin())) {
    std::cout << "success\n";
  } else {
    std::cout << "fail\n";
  }

  image2.write("fractal_gamma_pstl.png");

  return 0;
}
