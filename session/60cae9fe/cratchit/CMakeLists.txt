cmake_minimum_required(VERSION 3.25)

project(cratchit VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Warning flags shared by all targets
add_library(cratchit_warnings INTERFACE)
if(MSVC)
  target_compile_options(cratchit_warnings INTERFACE /W4 /permissive-)
else()
  target_compile_options(cratchit_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

# All application logic lives in the core library
add_library(cratchit_core
  src/core/version.cpp
)
target_include_directories(cratchit_core PUBLIC src)
target_link_libraries(cratchit_core PRIVATE cratchit_warnings)
target_compile_definitions(cratchit_core PRIVATE CRATCHIT_VERSION="${PROJECT_VERSION}")

# Thin executable on top of the core library
add_executable(cratchit
  src/main.cpp
)
target_link_libraries(cratchit PRIVATE cratchit_core cratchit_warnings)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(test)
endif()
