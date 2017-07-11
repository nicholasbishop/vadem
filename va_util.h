// Copyright 2017 Neverware

#ifndef VA_UTIL_H_
#define VA_UTIL_H_

#include <va/va.h>

namespace vadem {

void check_status(const VAStatus status);

std::size_t va_image_check_offset(const VAImage& image,
                                  const std::size_t offset);

void va_image_set_u8(const VAImage& image,
                     uint8_t* const mem,
                     const std::size_t offset,
                     const uint8_t val);

uint8_t va_image_get_u8(const VAImage& image,
                        uint8_t* const mem,
                        const std::size_t offset);

}

#endif  // VA_UTIL_H_
