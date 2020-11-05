//==============================================================
// Copyright (c) 2019-2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// =============================================================

#ifndef _GAMMA_UTILS_IMG_HPP
#define _GAMMA_UTILS_IMG_HPP

#include "ImgPixel.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>

// Image class definition
template <typename Format>
class Img {
 private:
  Format _format;
  std::int32_t _width;
  std::int32_t _height;
  std::vector<ImgPixel> _pixels;

  using Iterator = std::vector<ImgPixel>::iterator;
  using ConstIterator = std::vector<ImgPixel>::const_iterator;

 public:
  /////////////////////
  // SPECIAL METHODS //
  /////////////////////

  Img(std::int32_t width, std::int32_t height);

  void reset(std::int32_t width, std::int32_t height);

  ///////////////
  // ITERATORS //
  ///////////////

  Iterator begin() noexcept;
  Iterator end() noexcept;
  ConstIterator begin() const noexcept;
  ConstIterator end() const noexcept;
  ConstIterator cbegin() const noexcept;
  ConstIterator cend() const noexcept;

  /////////////
  // GETTERS //
  /////////////

  std::int32_t width() const noexcept;
  std::int32_t height() const noexcept;

  ImgPixel const* data() const noexcept;
  ImgPixel* data() noexcept;

  ///////////////////
  // FUNCTIONALITY //
  ///////////////////

  void write(std::string const& filename) const;

  template <typename Functor>
  void fill(Functor f);
  void fill(ImgPixel pixel);
  void fill(ImgPixel pixel, std::int32_t row, std::int32_t col);
};

///////////////////////////////////////////////
// IMG CLASS IMPLEMENTATION: SPECIAL METHODS //
///////////////////////////////////////////////

template <typename Format>
Img<Format>::Img(std::int32_t width, std::int32_t height)
    : _format(width, height) {
  _pixels.resize(width * height);

  _width = width;
  _height = height;
}

template <typename Format>
void Img<Format>::reset(std::int32_t width, std::int32_t height) {
  _pixels.resize(width * height);

  _width = width;
  _height = height;

  _format.reset(width, height);
}

/////////////////////////////////////////
// IMG CLASS IMPLEMENTATION: ITERATORS //
/////////////////////////////////////////

template <typename Format>
typename Img<Format>::Iterator Img<Format>::begin() noexcept {
  return _pixels.begin();
}

template <typename Format>
typename Img<Format>::Iterator Img<Format>::end() noexcept {
  return _pixels.end();
}

template <typename Format>
typename Img<Format>::ConstIterator Img<Format>::begin() const noexcept {
  return _pixels.begin();
}

template <typename Format>
typename Img<Format>::ConstIterator Img<Format>::end() const noexcept {
  return _pixels.end();
}

template <typename Format>
typename Img<Format>::ConstIterator Img<Format>::cbegin() const noexcept {
  return _pixels.begin();
}

template <typename Format>
typename Img<Format>::ConstIterator Img<Format>::cend() const noexcept {
  return _pixels.end();
}

///////////////////////////////////////
// IMG CLASS IMPLEMENTATION: GETTERS //
///////////////////////////////////////

template <typename Format>
std::int32_t Img<Format>::width() const noexcept {
  return _width;
}

template <typename Format>
std::int32_t Img<Format>::height() const noexcept {
  return _height;
}

template <typename Format>
ImgPixel const* Img<Format>::data() const noexcept {
  return _pixels.data();
}

template <typename Format>
ImgPixel* Img<Format>::data() noexcept {
  return _pixels.data();
}

/////////////////////////////////////////////
// IMG CLASS IMPLEMENTATION: FUNCTIONALITY //
/////////////////////////////////////////////

template <typename Format>
void Img<Format>::write(std::string const& filename) const {
  if (_pixels.empty()) {
    std::cerr << "Img::write:: image is empty" << std::endl;
    return;
  }

  std::ofstream filestream(filename, std::ios::binary);

  _format.write(filestream, *this);
}

template <typename Format>
template <typename Functor>
void Img<Format>::fill(Functor f) {
  if (_pixels.empty()) {
    std::cerr << "Img::fill(Functor): image is empty" << std::endl;
    return;
  }

  for (auto& pixel : _pixels) f(pixel);
}

template <typename Format>
void Img<Format>::fill(ImgPixel pixel) {
  if (_pixels.empty()) {
    std::cerr << "Img::fill(ImgPixel): image is empty" << std::endl;
    return;
  }

  std::fill(_pixels.begin(), _pixels.end(), pixel);
}

template <typename Format>
void Img<Format>::fill(ImgPixel pixel, int row, int col) {
  if (_pixels.empty()) {
    std::cerr << "Img::fill(ImgPixel): image is empty" << std::endl;
    return;
  }

  if (row >= _height || row < 0 || col >= _width || col < 0) {
    std::cerr << "Img::fill(ImgPixel, int, int): out of range" << std::endl;
    return;
  }

  _pixels.at(row * _width + col) = pixel;
}

#endif  // _GAMMA_UTILS_IMG_HPP
