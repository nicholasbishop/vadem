// Copyright 2017 Neverware

#include <stdexcept>

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

}
