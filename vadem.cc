#include <cassert>
#include <iostream>
#include <sstream>

#include <fcntl.h>

#include <va/va.h>
#include <va/va_drm.h>

#include "png.hpp"

static VADisplay g_display = nullptr;

template <typename T>
std::string hex_str(const T& t) {
  std::stringstream ss;
  ss << "0x" << std::hex << t;
  return ss.str();
}

template <typename A, typename B>
static void assert_equal(const A& a, const B& b) {
  if (a != b) {
    throw std::runtime_error(std::to_string(a) + " != " + std::to_string(b));
  }
}

static void check_status(const VAStatus status) {
  if (status != VA_STATUS_SUCCESS) {
    throw std::runtime_error("VAStatus: " + hex_str(status));
  }
}

static std::size_t va_image_check_offset(const VAImage& image,
                                         const std::size_t offset) {
  if (offset >= image.data_size) {
    throw std::runtime_error("image offset out of bounds: " +
                             std::to_string(offset) + " >= " +
                             std::to_string(image.data_size));
  }
  return offset;
}

static void va_image_set_u8(const VAImage& image,
                            uint8_t* const mem,
                            const std::size_t offset,
                            const uint8_t val) {
  va_image_check_offset(image, offset);
  mem[offset] = val;
}

static uint8_t va_image_get_u8(const VAImage& image,
                               uint8_t* const mem,
                               const std::size_t offset) {
  va_image_check_offset(image, offset);
  return mem[offset];
}

class ScopedBufferMap {
 public:
  ScopedBufferMap(const VABufferID buf) : buf_(buf) {
    void** vmem = reinterpret_cast<void**>(&mem_);
    check_status(vaMapBuffer(g_display, buf_, vmem));
  }

  ~ScopedBufferMap() { check_status(vaUnmapBuffer(g_display, buf_)); }

  uint8_t* data() { return mem_; }

 private:
  const VABufferID buf_;
  uint8_t* mem_;
};

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

  png::rgb_pixel to_rgb() const {
    return {red_u8(), green_u8(), blue_u8()};
  }

  T red() const { return 1.164 * (Y - 16) + 1.596 * (Cr - 128); }

  T green() const {
    return 1.164 * (Y - 16) - 0.813 * (Cr - 128) - 0.392 * (Cb - 128);
  }

  T blue() const { return 1.164 * (Y - 16) + 2.017 * (Cb - 128); }

  uint8_t red_u8() const {
    return red();
  }

  uint8_t green_u8() const {
    return green();
  }

  uint8_t blue_u8() const {
    return blue();
  }

  T Y, Cb, Cr;
};

class Nv12Buffer {
 public:
  using Offset = std::size_t;

  Nv12Buffer(const VAImage& image)
      : image(image),
        bufmap(image.buf),
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

  void set_pixel(const Offset x, const Offset y, const YCbCr& color) const {
    va_image_set_u8(image, mem, offset_Y(x, y), color.Y);
    va_image_set_u8(image, mem, offset_Cb(x, y), color.Cb);
    va_image_set_u8(image, mem, offset_Cr(x, y), color.Cr);
  }

 private:
  const VAImage image;
  ScopedBufferMap bufmap;
  uint8_t* const mem;

  const Offset w, h;
  const Offset half_w, half_h;
  const Offset plane2;
};

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

static png::image<png::rgb_pixel> va_image_nv12_copy_to_png(const VAImage& src) {
  const std::size_t w = src.width;
  const std::size_t h = src.height;

  assert_equal(src.format.fourcc, (unsigned)VA_FOURCC_NV12);
  assert_equal(src.format.bits_per_pixel, 12u);

  png::image<png::rgb_pixel> dst(w, h);

  Nv12Buffer buf(src);

  for (uint32_t y = 0; y < h; y++) {
    for (uint32_t x = 0; x < w; x++) {
      YCbCr pixel = buf.get_pixel(x, y);
      dst.set_pixel(x, y, pixel.to_rgb());
    }
  }

  return dst;
}

static void va_image_nv12_copy_from_png(const VAImage& dst,
                                        const png::image<png::rgb_pixel>& src) {
  const std::size_t w = src.get_width();
  const std::size_t h = src.get_height();

  assert_equal(w, dst.width);
  assert_equal(h, dst.height);

  Nv12Buffer buf(dst);

  for (uint32_t y = 0; y < h; y++) {
    for (uint32_t x = 0; x < w; x++) {
      const auto src_color = src.get_pixel(x, y);

      buf.set_pixel(x, y, YCbCr::from_rgb(src_color));
    }
  }
}

void va_image_rgb_copy_from_png(const VAImage& dst,
                                const png::image<png::rgb_pixel>& src) {
  ScopedBufferMap bufmap(dst.buf);
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

static png::image<png::rgb_pixel> va_image_rgb_copy_to_png(const VAImage& src) {
  assert_equal(src.format.fourcc, (unsigned)VA_FOURCC_RGBX);
  assert((src.format.depth == 24u) || (src.format.depth == 32u));

  png::image<png::rgb_pixel> dst(src.width, src.height);

  ScopedBufferMap bufmap(src.buf);
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

static void va_image_save(const VAImage& src, const std::string& filename) {
  std::cout << "writing VAImage to " << filename << std::endl;
  auto output_png = ((src.format.fourcc == VA_FOURCC_NV12) ?
                     va_image_nv12_copy_to_png(src) :
                     va_image_rgb_copy_to_png(src));
  output_png.write(filename);
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
