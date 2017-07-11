// Copyright 2017 Neverware

#ifndef NV12_H_
#define NV12_H_

#include <va/va.h>

#include "scoped_buffer_map.h"
#include "va_util.h"

namespace vadem {

class Nv12Buffer {
 public:
  using Offset = std::size_t;

  Nv12Buffer(VADisplay display, const VAImage& image)
      : image(image),
        bufmap(display, image.buf),
        mem(bufmap.data()),
        w(image.width),
        h(image.height),
        half_w(w / 2),
        half_h(h / 2),
        plane2(w * h) {
    assert_equal(image.format.fourcc, (unsigned)VA_FOURCC_NV12);
    assert_equal(image.num_planes, 2u);

    // Easier to reason about
    assert_equal(half_w * 2, w);
    assert_equal(half_h * 2, h);
  }

  Offset offset_Y(const Offset x, const Offset y) const {
    return va_image_check_offset(image, y * w + x);
  }

  Offset offset_Cb(const Offset x, const Offset y) const {
    return va_image_check_offset(image, plane2 + ((y / 2) * half_w + (x / 2)));
  }

  Offset offset_Cr(const Offset x, const Offset y) const {
    return va_image_check_offset(image, offset_Cb(x, y) + 1);
  }

  YCbCr get_pixel(const Offset x, const Offset y) const {
    return YCbCr(va_image_get_u8(image, mem, offset_Y(x, y)),
                 va_image_get_u8(image, mem, offset_Cb(x, y)),
                 va_image_get_u8(image, mem, offset_Cr(x, y)));
  }

  void set_pixel(const Offset x, const Offset y, const YCbCr& color) {
    va_image_set_u8(image, mem, offset_Y(x, y), color.Y);
    va_image_set_u8(image, mem, offset_Cb(x, y), color.Cb);
    va_image_set_u8(image, mem, offset_Cr(x, y), color.Cr);
  }

  void set_u8(const Offset offset, const uint8_t val) {
    va_image_set_u8(image, mem, offset, val);
  }

  void fill_u8(const uint8_t val) { memset(mem, val, image.data_size); }

  uint8_t* data() { return mem; }

 private:
  const VAImage image;
  ScopedBufferMap bufmap;
  uint8_t* const mem;

  const Offset w, h;
  const Offset half_w, half_h;
  const Offset plane2;
};
}

#endif  // NV12_H_
