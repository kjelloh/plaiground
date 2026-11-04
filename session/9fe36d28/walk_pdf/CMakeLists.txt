cmake_minimum_required(VERSION 3.25)

project(walk_pdf VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Warnings are errors for all targets (CMake's own setting, so it applies to
# every compiler). Override once with: cmake --compile-no-warning-as-error ...
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

# Warning flags shared by all targets
add_library(walk_pdf_warnings INTERFACE)
if(MSVC)
  target_compile_options(walk_pdf_warnings INTERFACE /W4 /permissive-)
else()
  target_compile_options(walk_pdf_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

# Build MANUAL.md into the program as bytes; CMake re-runs when the manual changes
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS MANUAL.md)
file(READ MANUAL.md manual_hex HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," manual_bytes "${manual_hex}")
configure_file(src/manual_text.cpp.in generated/manual_text.cpp @ONLY)

# The walker and other application logic live in the core library
add_library(walk_pdf_core
  src/document.cpp
  src/path.cpp
  src/scanner.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/generated/manual_text.cpp
)
target_include_directories(walk_pdf_core PUBLIC src)
target_link_libraries(walk_pdf_core PRIVATE walk_pdf_warnings)

# Thin executable on top of the core library
add_executable(walk_pdf
  src/main.cpp
)
target_link_libraries(walk_pdf PRIVATE walk_pdf_core walk_pdf_warnings)
target_compile_definitions(walk_pdf PRIVATE WALK_PDF_VERSION="${PROJECT_VERSION}")

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(test)
endif()
