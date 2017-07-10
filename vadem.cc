#include <cassert>
#include <iostream>
#include <sstream>

#include <fcntl.h>

#include <va/va.h>
#include <va/va_drm.h>

#include "png.hpp"

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

static void va_image_set_u8(const VAImage& image, uint8_t* const mem,
                            const std::size_t offset, const uint8_t val) {
  if (offset < image.data_size) {
    mem[offset] = val;
  } else {
    throw std::runtime_error("image offset out of bounds: " +
                             std::to_string(offset) + " >= " +
                             std::to_string(image.data_size));
  }
}

class ScopedBufferMap {
 public:
  ScopedBufferMap(VADisplay display, const VABufferID buf)
      : display_(display), buf_(buf) {
    void** vmem = reinterpret_cast<void**>(&mem_);
    check_status(vaMapBuffer(display_, buf_, vmem));
  }

  ~ScopedBufferMap() {
    check_status(vaUnmapBuffer(display_, buf_));
  }

  uint8_t* data() {
    return mem_;
  }

 private:
  VADisplay display_;
  const VABufferID buf_;
  uint8_t* mem_;
};

int main() {
  const char* device_path = "/dev/dri/renderD128";
  const int fd = open(device_path, 0);
  if (fd == -1) {
    std::cerr << "failed to open " << device_path << std::endl;
    return 1;
  }

  VADisplay display = vaGetDisplayDRM(fd);

  int major = 0;
  int minor = 0;
  check_status(vaInitialize(display, &major, &minor));

  std::cout << "libva initialized, version "
            << major << "." << minor
            << std::endl;

  const unsigned int surface_format = VA_RT_FORMAT_RGB32;
  const unsigned int width = 640;
  const unsigned int height = 480;
  VASurfaceID surface_id = 0;
  check_status(vaCreateSurfaces(
      display, surface_format, width, height, &surface_id, 1, nullptr, 0));

  png::image<png::rgba_pixel> input_png("data/color_bus_buddy.png");
  assert(input_png.get_width() == width);
  assert(input_png.get_height() == height);

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
  VAImage image1;
  check_status(vaCreateImage(
      display,
      &image_format,
      width,
      height,
      &image1));

  {
    ScopedBufferMap bufmap(display, image1.buf);
    uint8_t *mem = bufmap.data();

    for (uint32_t y = 0; y < height; y++) {
      for (uint32_t x = 0; x < width; x++) {
        const uint32_t offset = (y * width + x) * 4;
        const auto pixel = input_png.get_pixel(x, y);
        va_image_set_u8(image, mem, offset + 0, pixel.red);
        va_image_set_u8(image, mem, offset + 1, pixel.green);
        va_image_set_u8(image, mem, offset + 2, pixel.blue);
        va_image_set_u8(image, mem, offset + 3, pixel.alpha);
      }
    }
  }

  VAImage image2;
  check_status(vaPutImage(display,
                          surface_id,
                          image2.image_id,
                          0, 0, width, height,
                          0, 0, width, height));

  {
    ScopedBufferMap bufmap(display, image1.buf);
    uint8_t *mem = bufmap.data();

  check_status(vaDeriveImage(display, surface_id, &image2));

  mem = nullptr;
  check_status(vaMapBuffer(display, image2.buf, reinterpret_cast<void**>(&mem)));

  

  check_status(vaUnmapBuffer(display, image2.buf));

  check_status(vaTerminate(display));

  return 0;
}
