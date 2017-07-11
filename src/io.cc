// Copyright 2017 Neverware

#include <iostream>

#include "src/io.h"
#include "src/nv12.h"

namespace vadem {

png::image<png::rgb_pixel> va_image_rgb_copy_to_png(const VAImage& src) {
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

png::image<png::rgb_pixel> va_image_nv12_copy_to_png(
    const VAImage& src) {
  const std::size_t w = src.width;
  const std::size_t h = src.height;

  assert_equal(src.format.fourcc, (unsigned)VA_FOURCC_NV12);
  assert_equal(src.format.bits_per_pixel, 12u);

  png::image<png::rgb_pixel> dst(w, h);

  Nv12Buffer buf(g_display, src);

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

void va_image_save(const VAImage& src, const std::string& filename) {
  std::cout << "writing VAImage to " << filename << std::endl;
  auto output_png =
      ((src.format.fourcc == VA_FOURCC_NV12) ? va_image_nv12_copy_to_png(src)
                                             : va_image_rgb_copy_to_png(src));
  output_png.write(filename);
}
}
