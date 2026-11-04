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

add_executable(walk_pdf
  src/main.cpp
)
target_link_libraries(walk_pdf PRIVATE walk_pdf_warnings)
target_compile_definitions(walk_pdf PRIVATE WALK_PDF_VERSION="${PROJECT_VERSION}")
