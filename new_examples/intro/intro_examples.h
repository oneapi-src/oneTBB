/*
    Copyright (c) 2024 Intel Corporation

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

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace ch01 {

class Image {
public:
  union Pixel {
    std::uint8_t bgra[4];
    std::uint32_t value;
    Pixel() {}
    template <typename T> Pixel(T b, T g, T r) {
      bgra[0] = (std::uint8_t)b, bgra[1] = (std::uint8_t)g, bgra[2] = (std::uint8_t)r, bgra[3] = 0;
    }
  };

  Image(const std::string &n, int w = 1920, int h = 1080) : myName(n) {
    reset(w, h);
  }

  std::string name() const { return myName; }
  std::string setName(const std::string &n) { return myName = n; }

  int width() const { return myWidth; }
  int height() const { return myHeight; }

  void write(const char* fname) const {
    if(myData.empty()) {
      std::cout << "Warning: Image is empty.\n";
      return;
    }
    std::ofstream stream{fname};
    stream.write((char*)&file.type, file.sizeRest);
    stream.write((char*)&info, info.size);
    stream.write((char*)myData[0].bgra, myData.size()*sizeof(myData[0]));
  }

  void fill(std::uint8_t r, std::uint8_t g, std::uint8_t b, int x = -1, int y = -1) {
    if(myData.empty())
      return;

    if(x < 0 && y < 0) //fill whole Image
      std::fill(myData.begin(), myData.end(), Pixel(b, g, r));
    else {
      auto& bgra = myData[myWidth*x + y].bgra;
      bgra[3] = 0, bgra[2] = r, bgra[1] = g, bgra[0] = b;
    }
  }

  template <typename F>
  void fill(F f) {
    if(myData.empty())
      reset(myWidth, myHeight);

    int i = -1;
    int w = this->myWidth;
    std::for_each(myData.begin(), myData.end(), [&i, w, f](Image::Pixel& p) {
      ++i;
      int x = i / w, y = i % w;
      auto val = f(x, y);
      if(val > 255)
        val = 255;
      p = Image::Pixel(val, val, val);
    });
  }

  std::vector<Pixel*>& rows() { return myRows; }

private:
  void reset(int w, int h) {
    if(w <= 0 || h <= 0) {
      std::cout << "Warning: Invalid Image size.\n";
      return;
    }

    myWidth = w, myHeight = h;

    //reset raw data
    myData.resize(myWidth*myHeight);
    myRows.resize(myHeight);

    //reset rows
    for(int i = 0; i < myRows.size(); ++i)
      myRows[i] = &myData[0]+i*myWidth;

    myPadSize = (4-(w*sizeof(myData[0]))%4)%4;
    int sizeData = w*h*sizeof(myData[0]) + h*myPadSize;
    int sizeAll = sizeData + sizeof(file) + sizeof(info);

    //BITMAPFILEHEADER
    file.sizeRest = 14;
    file.type = 0x4d42; //same as 'BM' in ASCII
    file.size = sizeAll;
    file.reserved = 0;
    file.offBits = 54;

    //BITMAPINFOHEADER
    info.size = 40;
    info.width = w;
    info.height = h;
    info.planes = 1;
    info.bitCount = 32;
    info.compression = 0;
    info.sizeImage = sizeData;
    info.yPelsPerMeter = 0;
    info.xPelsPerMeter = 0;
    info.clrUsed = 0;
    info.clrImportant = 0;
  }

private:
  //don't allow copying
  Image(const Image&);
  void operator=(const Image&);

private:
  std::string myName;
  int myWidth;
  int myHeight;
  int myPadSize;

  std::vector<Pixel> myData; //raw raster data
  std::vector<Pixel*> myRows;

  //data structures 'file' and 'info' are using to store an Image as BMP file
  //for more details see https://en.wikipedia.org/wiki/BMP_file_format
  using BITMAPFILEHEADER = struct {
    std::uint16_t sizeRest; // field is not from specification, 
              // was added for alignemt. store size of rest of the fields
    std::uint16_t type;
    std::uint32_t size;
    std::uint32_t reserved; 
    std::uint32_t offBits;
  };
  BITMAPFILEHEADER file;

  using BITMAPINFOHEADER = struct {
    std::uint32_t size;
    std::int32_t width;
    std::int32_t height;
    std::uint16_t planes;
    std::uint16_t bitCount;
    std::uint32_t compression;
    std::uint32_t sizeImage;
    std::int32_t xPelsPerMeter;
    std::int32_t yPelsPerMeter;
    std::uint32_t clrUsed;
    std::uint32_t clrImportant;
  };
  BITMAPINFOHEADER info;
};

const int IMAGE_WIDTH = 800;
const int IMAGE_HEIGHT = 800;
const int MAX_BGR_VALUE = 255;

//! Fractal class
class Fractal {
public:
  //! Constructor
  Fractal(int x, int y, double m = 2000000.0): mySize{x, y}, myMagn(m) {}
  //! One pixel calculation routine
  double calcOnePixel(int x0, int y0) {
    double fx0 = double(x0) - double(mySize[0]) / 2;
    double fy0 = double(y0) - double(mySize[1]) / 2;
    fx0 = fx0 / myMagn + cx;
    fy0 = fy0 / myMagn + cy;

    double res = 0, x = 0, y = 0;
    for(int iter = 0; x*x + y*y <= 4 && iter < maxIter; ++iter) {
      const double val = x*x - y*y + fx0;
      y = 2*x*y + fy0, x = val;
      res += exp(-sqrt(x*x+y*y));
    }
    return res;
  }

private:
  //! Size of the Fractal area
  const int mySize[2];
  //! Fractal properties
  double cx = -0.7436;
  const double cy = 0.1319;
  const double myMagn;
  const int maxIter = 1000;
};

static std::shared_ptr<Image> makeFractalImage(double magn = 2000000) {
  const std::string name = std::string("fractal_") + std::to_string((int)magn);
  auto image_ptr = std::make_shared<Image>(name, IMAGE_WIDTH, IMAGE_HEIGHT);
  Fractal fr(image_ptr->width(), image_ptr->height(), magn);
  image_ptr->fill([&fr](int x, int y) { return fr.calcOnePixel(x, y); });
  return image_ptr;
}

}

