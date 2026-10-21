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

# Build MANUAL.md into the program as bytes; CMake re-runs when the manual changes
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS MANUAL.md)
file(READ MANUAL.md manual_hex HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," manual_bytes "${manual_hex}")
configure_file(src/core/manual_text.cpp.in generated/manual_text.cpp @ONLY)

# All application logic lives in the core library
add_library(cratchit_core
  src/core/charset/cp437.cpp
  src/core/charset/utf8.cpp
  src/core/data_folder.cpp
  src/core/help.cpp
  src/core/manual.cpp
  src/core/path_editor.cpp
  src/core/path_value.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/generated/manual_text.cpp
  src/core/repl.cpp
  src/core/terminal.cpp
  src/core/version.cpp
  src/core/workspace.cpp
)
target_include_directories(cratchit_core PUBLIC src)
target_link_libraries(cratchit_core PRIVATE cratchit_warnings)
if(WIN32)
  # SHGetKnownFolderPath and CoTaskMemFree, for the default workspace folder
  target_link_libraries(cratchit_core PRIVATE shell32 ole32)
endif()
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
