// Copyright 2017 Neverware

#ifndef COLOR_H_
#define COLOR_H_

#include "png.hpp"
#include "util.h"

namespace vadem {

// Adapted from "YCbCr and YCCK Color Models",
// https://software.intel.com/en-us/node/503873
class YCbCr {
 public:
  using T = float;

  YCbCr() : Y(0), Cb(0), Cr(0) {}

  YCbCr(const T Y, const T Cb, const T Cr) : Y(Y), Cb(Cb), Cr(Cr) {}

  static YCbCr from_rgb(const T R, const T G, const T B) {
    const T Y = 0.257 * R + 0.504 * G + 0.098 * B + 16;
    const T Cb = -0.148 * R - 0.291 * G + 0.439 * B + 128;
    const T Cr = 0.439 * R - 0.368 * G - 0.071 * B + 128;
    return {Y, Cb, Cr};
  }

  static YCbCr from_rgb(const png::rgb_pixel& pixel) {
    return from_rgb(pixel.red, pixel.green, pixel.blue);
  }

  png::rgb_pixel to_rgb() const { return {red_u8(), green_u8(), blue_u8()}; }

  T red() const { return clamp_255(1.164 * (Y - 16) + 1.596 * (Cr - 128)); }

  T green() const {
    return clamp_255(1.164 * (Y - 16) - 0.813 * (Cr - 128) -
                     0.392 * (Cb - 128));
  }

  T blue() const { return clamp_255(1.164 * (Y - 16) + 2.017 * (Cb - 128)); }

  uint8_t red_u8() const { return red(); }

  uint8_t green_u8() const { return green(); }

  uint8_t blue_u8() const { return blue(); }

  static T clamp_255(const T val) { return clamp(val, 0, 255); }

  T Y, Cb, Cr;
};
}

#endif  // COLOR_H_
