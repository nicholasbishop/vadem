// Copyright 2017 Neverware

#include <iostream>

#include "src/io.h"
#include "src/nv12.h"

namespace vadem {

void va_image_dump(VADisplay display,
                   const VAImage& src,
                   const std::string& filename) {
  std::cout << "dumping VAImage to " << filename << std::endl;
  Nv12Buffer buf(display, src);
  FILE* file = fopen(filename.c_str(), "w");
  const std::size_t result = fwrite(buf.data(), 1, src.data_size, file);
  assert_equal(result, src.data_size);
  fclose(file);
}
}
