// Copyright 2017 Neverware

#ifndef SCOPED_BUFFER_MAP_H_
#define SCOPED_BUFFER_MAP_H_

#include <va/va.h>

#include "va_util.h"

namespace vadem {

class ScopedBufferMap {
 public:
  ScopedBufferMap(VADisplay display, const VABufferID buf) :
      display_(display), buf_(buf) {
    void** vmem = reinterpret_cast<void**>(&mem_);
    check_status(vaMapBuffer(display_, buf_, vmem));
  }

  ~ScopedBufferMap() { check_status(vaUnmapBuffer(display_, buf_)); }

  uint8_t* data() { return mem_; }

 private:
  VADisplay display_;
  const VABufferID buf_;
  uint8_t* mem_;
};

}

#endif  // SCOPED_BUFFER_MAP_H_
