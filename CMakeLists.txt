cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wmissing-declarations")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wmissing-prototypes")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

include_directories(${PROJECT_SOURCE_DIR})
include_directories(SYSTEM png++)

add_executable(vadem src/io.cc src/va_util.cc src/vadem.cc)

target_link_libraries(vadem png va va-drm)