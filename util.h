// Copyright 2017 Neverware

#ifndef UTIL_H_
#define UTIL_H_

#include <sstream>

namespace vadem {

template <typename T>
std::string hex_str(const T& t) {
  std::stringstream ss;
  ss << "0x" << std::hex << t;
  return ss.str();
}

template <typename A, typename B>
void assert_equal(const A& a, const B& b) {
  if (a != b) {
    throw std::runtime_error(std::to_string(a) + " != " + std::to_string(b));
  }
}

template <typename T, typename L, typename H>
T clamp(const T val, const L min, const H max) {
  if (val < min) {
    return min;
  } else if (val > max) {
    return max;
  } else {
    return val;
  }
}

}

#endif  // UTIL_H_
