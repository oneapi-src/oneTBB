//==============================================================
// Copyright (c) 2019-2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// =============================================================

#ifndef _GAMMA_UTILS_IMGFORMAT_HPP
#define _GAMMA_UTILS_IMGFORMAT_HPP

#include "ImgPixel.hpp"

#include <fstream>

namespace ImgFormat {

// struct to store an image in BMP format
struct BMP {
 private:
  using FileHeader = struct {
    // not from specification
    // was added for alignment
    // store size of rest of the fields
    std::uint16_t sizeRest;  // file header size in bytes

    std::uint16_t type;
    std::uint32_t size;  // file size in bytes
    std::uint32_t reserved;
    std::uint32_t offBits;  // cumulative header size in bytes
  };

  using InfoHeader = struct {
    // from specification
    // store size of rest of the fields
    std::uint32_t size;  // info header size in bytes

    std::int32_t width;   // image width in pixels
    std::int32_t height;  // image height in pixels
    std::uint16_t planes;
    std::uint16_t bitCount;      // color depth
    std::uint32_t compression;   // compression
    std::uint32_t sizeImage;     // image map size in bytes
    std::int32_t xPelsPerMeter;  // pixel per metre (y axis)
    std::int32_t yPelsPerMeter;  // pixel per metre (y axis)
    std::uint32_t clrUsed;       // color palette (0 is default)
    std::uint32_t clrImportant;
  };

  FileHeader _fileHeader;
  InfoHeader _infoHeader;

 public:
  BMP(std::int32_t width, std::int32_t height) noexcept {
    reset(width, height);
  }

  void reset(std::int32_t width, std::int32_t height) noexcept {
    uint32_t padSize = (4 - (width * sizeof(ImgPixel)) % 4) % 4;
    uint32_t mapSize = width * height * sizeof(ImgPixel) + height * padSize;
    uint32_t allSize = mapSize + _fileHeader.sizeRest + _infoHeader.size;

    _fileHeader.sizeRest = 14;  // file header size in bytes
    _fileHeader.type = 0x4d42;
    _fileHeader.size = allSize;  // file size in bytes
    _fileHeader.reserved = 0;
    _fileHeader.offBits = 54;  // sizeRest + size -> 14 + 40 -> 54

    _infoHeader.size = 40;        // info header size in bytes
    _infoHeader.width = width;    // image width in pixels
    _infoHeader.height = height;  // image height in pixels
    _infoHeader.planes = 1;
    _infoHeader.bitCount = 32;        // color depth
    _infoHeader.compression = 0;      // compression
    _infoHeader.sizeImage = mapSize;  // image map size in bytes
    _infoHeader.xPelsPerMeter = 0;    // pixel per metre (x axis)
    _infoHeader.yPelsPerMeter = 0;    // pixel per metre (y axis)
    _infoHeader.clrUsed = 0;          // color palette (0 is default)
    _infoHeader.clrImportant = 0;
  }

  template <template <class> class Image, typename Format>
  void write(std::ofstream& ostream, Image<Format> const& image) const {
    ostream.write(reinterpret_cast<char const*>(&_fileHeader.type),
                  _fileHeader.sizeRest);

    ostream.write(reinterpret_cast<char const*>(&_infoHeader),
                  _infoHeader.size);

    ostream.write(reinterpret_cast<char const*>(image.data()),
                  image.width() * image.height() * sizeof(image.data()[0]));
  }

  FileHeader const& fileHeader() const noexcept { return _fileHeader; }
  InfoHeader const& infoHeader() const noexcept { return _infoHeader; }
};

}  // namespace ImgFormat

#endif  // _GAMMA_UTILS_IMGFORMAT_HPP
