// Copyright 2017 Neverware

#ifndef SRC_IO_H_
#define SRC_IO_H_

#include <string>

#include <va/va.h>

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

void va_image_save(const VAImage& src, const std::string& filename);
}

#endif  // SRC_IO_H_
