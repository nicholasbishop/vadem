// Copyright 2017 Neverware

#include <cassert>
#include <iostream>
#include <sstream>

#include <fcntl.h>

#include <va/va.h>
#include <va/va_drm.h>

#include "color.h"
#include "io.h"
#include "nv12.h"
#include "png.hpp"
#include "util.h"

static VADisplay g_display = nullptr;

using namespace vadem;

VAImage va_image_create_rgb(const int width, const int height) {
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
  check_status(vaCreateImage(g_display, &image_format, width, height, &image));

  return image;
}

static VAImage va_image_create_nv12(const int width, const int height) {
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
  check_status(vaCreateImage(g_display, &image_format, width, height, &image));

  return image;
}

VAImage va_image_nv12_gen_CbCr_gradient(const float Y) {
  const std::size_t w = 512;
  const std::size_t half_w = w / 2;
  const VAImage image = va_image_create_nv12(w, w);
  Nv12Buffer buf(g_display, image);

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

VAImage va_image_nv12_gen_Y_gradient() {
  const std::size_t w = 128;
  const VAImage image = va_image_create_nv12(w, w);
  Nv12Buffer buf(g_display, image);
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

static void va_image_nv12_copy_from_png(const VAImage& dst,
                                        const png::image<png::rgb_pixel>& src) {
  const std::size_t w = src.get_width();
  const std::size_t h = src.get_height();

  assert_equal(w, dst.width);
  assert_equal(h, dst.height);

  Nv12Buffer buf(g_display, dst);

  for (uint32_t y = 0; y < h; y++) {
    for (uint32_t x = 0; x < w; x++) {
      const auto src_color = src.get_pixel(x, y);

      buf.set_pixel(x, y, YCbCr::from_rgb(src_color));
    }
  }
}

void va_image_rgb_copy_from_png(const VAImage& dst,
                                const png::image<png::rgb_pixel>& src) {
  ScopedBufferMap bufmap(g_display, dst.buf);
  uint8_t* mem = bufmap.data();

  assert_equal(src.get_width(), dst.width);
  assert_equal(src.get_height(), dst.height);
  assert_equal(dst.format.depth, 32u);

  for (uint32_t y = 0; y < dst.height; y++) {
    for (uint32_t x = 0; x < dst.width; x++) {
      const uint32_t offset = (y * dst.width + x) * 4;
      const auto pixel = src.get_pixel(x, y);
      va_image_set_u8(dst, mem, offset + 0, pixel.red);
      va_image_set_u8(dst, mem, offset + 1, pixel.green);
      va_image_set_u8(dst, mem, offset + 2, pixel.blue);
    }
  }
}

int main() {
  const char* device_path = "/dev/dri/renderD128";
  const int fd = open(device_path, 0);
  if (fd == -1) {
    std::cerr << "failed to open " << device_path << std::endl;
    return 1;
  }

  g_display = vaGetDisplayDRM(fd);

  int major = 0;
  int minor = 0;
  check_status(vaInitialize(g_display, &major, &minor));

  std::cout << "libva initialized, version " << major << "." << minor
            << std::endl;

  // Load test image
  const std::string& input_path = "data/color_bus_buddy_256_square.png";
  std::cout << "loading test image: " << input_path << std::endl;
  png::image<png::rgb_pixel> input_png(input_path);
  const auto width = input_png.get_width();
  const auto height = input_png.get_height();

  const auto gradient_image = va_image_nv12_gen_CbCr_gradient(128);
  // const auto gradient_image = va_image_nv12_gen_Y_gradient();
  va_image_save(gradient_image, "gradient.png");
  va_image_dump(g_display, gradient_image, "gradient.raw");

  // Copy test image into a new VAImage
  VAImage input_image = va_image_create_nv12(width, height);
  va_image_nv12_copy_from_png(input_image, input_png);

  // Sanity check: copy the original image back out to a new PNG file
  va_image_save(input_image, "input.png");

  // Create an empty surface
  const unsigned int surface_format = VA_RT_FORMAT_RGB32;
  VASurfaceID surface_id = 0;
  check_status(vaCreateSurfaces(g_display, surface_format, width, height,
                                &surface_id, 1, nullptr, 0));

  // Copy the test image into the surface
  check_status(vaPutImage(g_display, surface_id, input_image.image_id, 0, 0,
                          width, height, 0, 0, width, height));

  // Write the surface's image back out to a new PNG file
  VAImage surf_image;
  check_status(vaDeriveImage(g_display, surface_id, &surf_image));
  va_image_save(surf_image, "output.png");

  check_status(vaTerminate(g_display));

  return 0;
}
