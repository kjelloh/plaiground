cmake_minimum_required(VERSION 3.25)

project(cratchit VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Warnings are errors for all targets (CMake's own setting, so it applies to
# every compiler). Override once with: cmake --compile-no-warning-as-error ...
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

# Warning flags shared by all targets
add_library(cratchit_warnings INTERFACE)
if(MSVC)
  target_compile_options(cratchit_warnings INTERFACE /W4 /permissive-)
else()
  target_compile_options(cratchit_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

# Text transcoding: Unicode, UTF-8, UTF-16, CP437 and detection (from walk_pdf, session 9fe36d28)
add_library(charset
  src/charset/cp437.cpp
  src/charset/detect.cpp
  src/charset/utf16.cpp
  src/charset/utf8.cpp
)
target_include_directories(charset PUBLIC src)
target_link_libraries(charset PRIVATE cratchit_warnings)

add_executable(cratchit
  src/main.cpp
)
target_link_libraries(cratchit PRIVATE cratchit_warnings)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(test)
endif()
