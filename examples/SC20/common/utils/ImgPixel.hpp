//==============================================================
// Copyright (c) 2019-2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// =============================================================

#ifndef _GAMMA_UTILS_IMGPIXEL_HPP
#define _GAMMA_UTILS_IMGPIXEL_HPP

#include <cstdint>
#include <ostream>

// struct to store a pixel of image
struct ImgPixel {
  std::uint8_t b;
  std::uint8_t g;
  std::uint8_t r;
  std::uint8_t a;

  bool operator==(ImgPixel const& other) const {
    return (b == other.b) && (g == other.g) && (r == other.r) && (a == other.a);
  }

  bool operator!=(ImgPixel const& other) const { return !(*this == other); }

  void set(std::uint8_t blue, std::uint8_t green, std::uint8_t red,
           std::uint8_t alpha) {
    b = blue;
    g = green;
    r = red;
    a = alpha;
  }
};

std::ostream& operator<<(std::ostream& output, ImgPixel const& pixel) {
  return output << "(" << unsigned(pixel.r) << ", " << unsigned(pixel.g) << ", "
                << unsigned(pixel.b) << ", " << unsigned(pixel.a) << ")";
}

#endif  // _GAMMA_UTILS_IMGPIXEL_HPP
