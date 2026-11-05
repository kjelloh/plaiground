cmake_minimum_required(VERSION 3.25)

project(pyroceph VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Warnings are errors for all targets. Override once with:
# cmake --compile-no-warning-as-error ...
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

add_library(pyroceph_warnings INTERFACE)
if(MSVC)
  target_compile_options(pyroceph_warnings INTERFACE /W4 /permissive-)
else()
  target_compile_options(pyroceph_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

# Text transcoding, copied from walk_pdf (session 9fe36d28)
add_library(charset
  src/charset/utf8.cpp
)
target_include_directories(charset PUBLIC src)
target_link_libraries(charset PRIVATE pyroceph_warnings)

add_executable(pyroceph
  src/main.cpp
  src/terminal/raw_terminal.cpp
)
target_include_directories(pyroceph PRIVATE src)
target_link_libraries(pyroceph PRIVATE charset pyroceph_warnings)
