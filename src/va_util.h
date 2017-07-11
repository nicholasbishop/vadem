// Copyright 2017 Neverware

#ifndef VA_UTIL_H_
#define VA_UTIL_H_

#include <va/va.h>

namespace vadem {

void check_status(VAStatus status);

std::size_t va_image_check_offset(const VAImage& image,
                                  std::size_t offset);

void va_image_set_u8(const VAImage& image,
                     uint8_t* mem,
                     std::size_t offset,
                     uint8_t val);

uint8_t va_image_get_u8(const VAImage& image,
                        uint8_t* mem,
                        std::size_t offset);

VAImage va_image_create_rgb(VADisplay display, int width, int height);

VAImage va_image_create_nv12(VADisplay display, int width, int height);

VAImage va_image_nv12_gen_CbCr_gradient(VADisplay display, float Y);

VAImage va_image_nv12_gen_Y_gradient(VADisplay display);

}

#endif  // VA_UTIL_H_
