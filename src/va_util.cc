// Copyright 2017 Neverware

#include <stdexcept>

#include "nv12.h"
#include "util.h"
#include "va_util.h"

namespace vadem {

void check_status(const VAStatus status) {
  if (status != VA_STATUS_SUCCESS) {
    throw std::runtime_error("VAStatus: " + hex_str(status));
  }
}

std::size_t va_image_check_offset(const VAImage& image,
                                  const std::size_t offset) {
  if (offset >= image.data_size) {
    throw std::runtime_error("image offset out of bounds: " +
                             std::to_string(offset) + " >= " +
                             std::to_string(image.data_size));
  }
  return offset;
}

void va_image_set_u8(const VAImage& image,
                     uint8_t* const mem,
                     const std::size_t offset,
                     const uint8_t val) {
  va_image_check_offset(image, offset);
  mem[offset] = val;
}

uint8_t va_image_get_u8(const VAImage& image,
                        uint8_t* const mem,
                        const std::size_t offset) {
  va_image_check_offset(image, offset);
  return mem[offset];
}

VAImage va_image_create_rgb(VADisplay display, const int width, const int height) {
  VAImageFormat image_format{
      .fourcc = VA_FOURCC_RGBX,
      .byte_order = VA_LSB_FIRST,
      .bits_per_pixel = 24,
      .depth = 32,
      .red_mask = 0xff,
      .green_mask = 0xff00,
      .blue_mask = 0xff0000,
      .alpha_mask = 0x00000000,
  };
  VAImage image;
  check_status(vaCreateImage(display, &image_format, width, height, &image));

  return image;
}

VAImage va_image_create_nv12(VADisplay display, const int width, const int height) {
  VAImageFormat image_format{
      .fourcc = VA_FOURCC_NV12,
      .byte_order = VA_LSB_FIRST,
      .bits_per_pixel = 12,
      // These are only for RGB
      .depth = 0,
      .red_mask = 0,
      .green_mask = 0,
      .blue_mask = 0,
      .alpha_mask = 0,
  };
  VAImage image;
  check_status(vaCreateImage(display, &image_format, width, height, &image));

  return image;
}

VAImage va_image_nv12_gen_CbCr_gradient(VADisplay display, const float Y) {
  const std::size_t w = 512;
  const std::size_t half_w = w / 2;
  const VAImage image = va_image_create_nv12(display, w, w);
  Nv12Buffer buf(display, image);

  std::size_t offset = 0;

  for (std::size_t y = 0; y < w; y++) {
    for (std::size_t x = 0; x < w; x++) {
      buf.set_u8(y * w + x, Y);
      offset++;
    }
  }

  const std::size_t plane2 = w * w;
  for (std::size_t y = 0; y < half_w; y++) {
    for (std::size_t x = 0; x < half_w; x++) {
      const std::size_t Cb = (y * half_w + x) * 2;
      const std::size_t Cr = Cb + 1;
      buf.set_u8(plane2 + Cb, x);
      buf.set_u8(offset, x);
      offset++;
      buf.set_u8(plane2 + Cr, y);
      buf.set_u8(offset, y);
      offset++;
    }
  }

  assert_equal(offset, image.data_size);

  return image;
}

VAImage va_image_nv12_gen_Y_gradient(VADisplay display) {
  const std::size_t w = 128;
  const VAImage image = va_image_create_nv12(display, w, w);
  Nv12Buffer buf(display, image);
  buf.fill_u8(128);

  for (std::size_t y = 0; y < w; y++) {
    for (std::size_t x = 0; x < w; x++) {
      const auto fx = (x * 1.0) / w;
      const auto fy = (y * 1.0) / w;
      buf.set_u8(y * w + x, fx * fy * 256);
    }
  }

  return image;
}

}
