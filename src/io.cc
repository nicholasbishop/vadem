// Copyright 2017 Neverware

#include <iostream>

#include "src/io.h"
#include "src/nv12.h"

namespace vadem {

static png::image<png::rgb_pixel> va_image_rgb_copy_to_png(VADisplay display,
                                                           const VAImage& src) {
  assert_equal(src.format.fourcc, (unsigned)VA_FOURCC_RGBX);
  assert((src.format.depth == 24u) || (src.format.depth == 32u));

  png::image<png::rgb_pixel> dst(src.width, src.height);

  ScopedBufferMap bufmap(display, src.buf);
  uint8_t* mem = bufmap.data();

  const int bytes_per_pixel = src.format.depth / 8;

  for (uint32_t y = 0; y < src.height; y++) {
    for (uint32_t x = 0; x < src.width; x++) {
      const uint32_t offset = (y * src.width + x) * bytes_per_pixel;
      const auto pixel = png::rgb_pixel(va_image_get_u8(src, mem, offset + 0),
                                        va_image_get_u8(src, mem, offset + 1),
                                        va_image_get_u8(src, mem, offset + 2));
      dst.set_pixel(x, y, pixel);
    }
  }

  return dst;
}

static png::image<png::rgb_pixel> va_image_nv12_copy_to_png(
    VADisplay display,
    const VAImage& src) {
  const std::size_t w = src.width;
  const std::size_t h = src.height;

  assert_equal(src.format.fourcc, (unsigned)VA_FOURCC_NV12);
  assert_equal(src.format.bits_per_pixel, 12u);

  png::image<png::rgb_pixel> dst(w, h);

  Nv12Buffer buf(display, src);

  for (uint32_t y = 0; y < h; y++) {
    for (uint32_t x = 0; x < w; x++) {
      YCbCr pixel = buf.get_pixel(x, y);
      dst.set_pixel(x, y, pixel.to_rgb());
    }
  }

  return dst;
}

void va_image_dump(VADisplay display,
                   const VAImage& src,
                   const std::string& filename) {
  std::cout << "dumping VAImage to " << filename << std::endl;
  Nv12Buffer buf(display, src);
  FILE* file = fopen(filename.c_str(), "w");
  const std::size_t result = fwrite(buf.data(), 1, src.data_size, file);
  assert_equal(result, src.data_size);
  fclose(file);
}

void va_image_save(VADisplay display,
                   const VAImage& src,
                   const std::string& filename) {
  std::cout << "writing VAImage to " << filename << std::endl;
  auto output_png = ((src.format.fourcc == VA_FOURCC_NV12)
                         ? va_image_nv12_copy_to_png(display, src)
                         : va_image_rgb_copy_to_png(display, src));
  output_png.write(filename);
}

void va_image_rgb_copy_from_png(VADisplay display, const VAImage& dst,
                                const png::image<png::rgb_pixel>& src) {
  ScopedBufferMap bufmap(display, dst.buf);
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

void va_image_nv12_copy_from_png(VADisplay display, const VAImage& dst,
                                 const png::image<png::rgb_pixel>& src) {
  const std::size_t w = src.get_width();
  const std::size_t h = src.get_height();

  assert_equal(w, dst.width);
  assert_equal(h, dst.height);

  Nv12Buffer buf(display, dst);

  for (uint32_t y = 0; y < h; y++) {
    for (uint32_t x = 0; x < w; x++) {
      const auto src_color = src.get_pixel(x, y);

      buf.set_pixel(x, y, YCbCr::from_rgb(src_color));
    }
  }
}

}
