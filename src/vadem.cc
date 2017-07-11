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

using namespace vadem;

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

  std::cout << "libva initialized, version " << major << "." << minor
            << std::endl;

  // Load test image
  const std::string& input_path = "data/color_bus_buddy_256_square.png";
  std::cout << "loading test image: " << input_path << std::endl;
  png::image<png::rgb_pixel> input_png(input_path);
  const auto width = input_png.get_width();
  const auto height = input_png.get_height();

  const auto gradient_image = va_image_nv12_gen_CbCr_gradient(display, 128);
  // const auto gradient_image = va_image_nv12_gen_Y_gradient();
  va_image_save(display, gradient_image, "gradient.png");
  va_image_dump(display, gradient_image, "gradient.raw");

  // Copy test image into a new VAImage
  VAImage input_image = va_image_create_nv12(display, width, height);
  va_image_nv12_copy_from_png(display, input_image, input_png);

  // Sanity check: copy the original image back out to a new PNG file
  va_image_save(display, input_image, "input.png");

  // Create an empty surface
  const unsigned int surface_format = VA_RT_FORMAT_RGB32;
  VASurfaceID surface_id = 0;
  check_status(vaCreateSurfaces(display, surface_format, width, height,
                                &surface_id, 1, nullptr, 0));

  // Copy the test image into the surface
  check_status(vaPutImage(display, surface_id, input_image.image_id, 0, 0,
                          width, height, 0, 0, width, height));

  // Write the surface's image back out to a new PNG file
  VAImage surf_image;
  check_status(vaDeriveImage(display, surface_id, &surf_image));
  va_image_save(display, surf_image, "output.png");

  check_status(vaTerminate(display));

  return 0;
}
