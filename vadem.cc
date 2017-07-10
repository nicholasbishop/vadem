#include <cassert>
#include <iostream>
#include <sstream>

#include <fcntl.h>

#include <va/va.h>
#include <va/va_drm.h>

#include "png.hpp"

static VADisplay g_display = nullptr;

template<typename T>
std::string hex_str(const T& t) {
  std::stringstream ss;
  ss << "0x" << std::hex << t;
  return ss.str();
}

static void check_status(const VAStatus status) {
  if (status != VA_STATUS_SUCCESS) {
    throw std::runtime_error("VAStatus: " + hex_str(status));
  }
}

static void va_image_check_offset(const VAImage& image, uint8_t* const mem,
                                  const std::size_t offset) {
  if (offset >= image.data_size) {
    throw std::runtime_error("image offset out of bounds: " +
                             std::to_string(offset) + " >= " +
                             std::to_string(image.data_size));
  }
}

static void va_image_set_u8(const VAImage& image, uint8_t* const mem,
                            const std::size_t offset, const uint8_t val) {
  va_image_check_offset(image, mem, offset);
  mem[offset] = val;
}

static u8 va_image_get_u8(const VAImage& image, uint8_t* const mem,
                          const std::size_t offset) {
  va_image_check_offset(image, mem, offset);
  return mem[offset];
}

class ScopedBufferMap {
 public:
  ScopedBufferMap(const VABufferID buf) : buf_(buf) {
    void** vmem = reinterpret_cast<void**>(&mem_);
    check_status(vaMapBuffer(g_display, buf_, vmem));
  }

  ~ScopedBufferMap() {
    check_status(vaUnmapBuffer(g_display, buf_));
  }

  uint8_t* data() {
    return mem_;
  }

 private:
  const VABufferID buf_;
  uint8_t* mem_;
};

static VAImage va_image_create_rgba(const int width, const int height) {
  VAImageFormat image_format {
    .fourcc = VA_FOURCC_RGBA,
      .byte_order = VA_LSB_FIRST,
      .bits_per_pixel = 32,
      .depth = 32,
      .red_mask = 0xff,
      .green_mask = 0xff00,
      .blue_mask = 0xff0000,
      .alpha_mask = 0xff000000,
      };
  VAImage image;
  check_status(vaCreateImage(
      g_display,
      &image_format,
      width,
      height,
      &image));

  return image;
}

static void va_image_rgba_copy_from_png(
    const VAImage& dst, const png::image<png::rgba_pixel>& src) {
  ScopedBufferMap bufmap(dst.buf);
  uint8_t *mem = bufmap.data();

  assert(src.get_width() == dst.width);
  assert(src.get_height() == dst.height);

  for (uint32_t y = 0; y < dst.height; y++) {
    for (uint32_t x = 0; x < dst.width; x++) {
      const uint32_t offset = (y * dst.width + x) * 4;
      const auto pixel = src.get_pixel(x, y);
      va_image_set_u8(dst, mem, offset + 0, pixel.red);
      va_image_set_u8(dst, mem, offset + 1, pixel.green);
      va_image_set_u8(dst, mem, offset + 2, pixel.blue);
      va_image_set_u8(dst, mem, offset + 3, pixel.alpha);
    }
  }
}

static png::image<png::rgba_pixel> va_image_rgba_copy_to_png(
    const VAImage& src) {
  png::image<png::rgba_pixel> dst(surf_image.width, surf_image.height);

  ScopedBufferMap bufmap(src.buf);
  uint8_t *mem = bufmap.data();

  for (uint32_t y = 0; y < dst.height; y++) {
    for (uint32_t x = 0; x < dst.width; x++) {
      const uint32_t offset = (y * dst.width + x) * 4;
      const auto pixel = png::rgba_pixel(
          va_image_get_u8(src, mem, offset + 0),
          va_image_get_u8(src, mem, offset + 1),
          va_image_get_u8(src, mem, offset + 2),
          va_image_get_u8(src, mem, offset + 3));
      dst.set_pixel(x, y, pixel);
    }
  }

  return dst;
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

  std::cout << "libva initialized, version "
            << major << "." << minor
            << std::endl;

  const unsigned int surface_format = VA_RT_FORMAT_RGB32;
  const unsigned int width = 640;
  const unsigned int height = 480;
  VASurfaceID surface_id = 0;
  check_status(vaCreateSurfaces(
      g_display, surface_format, width, height, &surface_id, 1, nullptr, 0));

  png::image<png::rgba_pixel> input_png("data/color_bus_buddy.png");

  VAImage input_image = va_image_create_rgba(width, height);
  va_image_rgba_copy_from_png(input_image, input_png);

  check_status(vaPutImage(g_display,
                          surface_id,
                          input_image.image_id,
                          0, 0, width, height,
                          0, 0, width, height));

  VAImage surf_image;
  check_status(vaDeriveImage(g_display, surface_id, &surf_image));

  auto output_png = va_image_rgba_copy_to_png(surf_image);
  output_png.write("output.png");

  check_status(vaTerminate(g_display));

  return 0;
}
