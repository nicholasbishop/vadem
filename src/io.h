// Copyright 2017 Neverware

#ifndef SRC_IO_H_
#define SRC_IO_H_

#include <string>

#include <va/va.h>

#include "png.hpp"

namespace vadem {

// ImageMagick can display a raw NV12 file like so:
//
// display -size 512x512 -depth 8 -sample 4:2:0 -interlace plane
// yuv:coolfile.raw
//
// or try: http://rawpixels.net/
void va_image_dump(VADisplay display,
                   const VAImage& src,
                   const std::string& filename);

void va_image_save(VADisplay display, const VAImage& src, const std::string& filename);

void va_image_rgb_copy_from_png(VADisplay display, const VAImage& dst,
                                const png::image<png::rgb_pixel>& src);

void va_image_nv12_copy_from_png(VADisplay display, const VAImage& dst,
                                 const png::image<png::rgb_pixel>& src);

}

#endif  // SRC_IO_H_
