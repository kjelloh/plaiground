cmake_minimum_required(VERSION 3.25)

project(cratchit_ar VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Warning flags shared by all targets; warnings are errors
add_library(cratchit_warnings INTERFACE)
if(MSVC)
  target_compile_options(cratchit_warnings INTERFACE /W4 /WX /permissive-)
else()
  target_compile_options(cratchit_warnings INTERFACE -Wall -Wextra -Wpedantic -Werror)
endif()

# Build MANUAL.md into the program as bytes; CMake re-runs when the manual changes
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS MANUAL.md)
file(READ MANUAL.md manual_hex HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," manual_bytes "${manual_hex}")
configure_file(src/core/manual_text.cpp.in generated/manual_text.cpp @ONLY)

# Likewise the BAS to ÅR mapping into the program as bytes; CMake re-runs when it changes
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS Resources/bas_to_ar_2026.csv)
file(READ Resources/bas_to_ar_2026.csv layout_hex HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," layout_bytes "${layout_hex}")
configure_file(src/core/ar/layout_text.cpp.in generated/layout_text.cpp @ONLY)

# All application logic lives in the core library
add_library(cratchit_core
  src/core/amount.cpp
  src/core/ar/closing.cpp
  src/core/ar/disposition.cpp
  src/core/ar/equity.cpp
  src/core/ar/layout.cpp
  src/core/ar/notes.cpp
  src/core/ar/overview.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/generated/layout_text.cpp
  src/core/ar/statement.cpp
  src/core/ar/text.cpp
  src/core/ar/trace.cpp
  src/core/bas.cpp
  src/core/charset/cp437.cpp
  src/core/charset/detect.cpp
  src/core/charset/utf8.cpp
  src/core/csv.cpp
  src/core/date.cpp
  src/core/log.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/generated/manual_text.cpp
  src/core/sie/check.cpp
  src/core/sie/document.cpp
  src/core/sie/file.cpp
  src/core/sie/lexer.cpp
  src/core/terminal_encoding.cpp
)
target_include_directories(cratchit_core PUBLIC src)
target_link_libraries(cratchit_core PRIVATE cratchit_warnings)

# LOG_DEVELOPMENT_TRACE is compiled in when CRATCHIT_DEVELOPMENT_TRACE is ON,
# left out when OFF, and DEBUG (the default) compiles it into Debug builds only
set(CRATCHIT_DEVELOPMENT_TRACE DEBUG CACHE STRING "Compile in LOG_DEVELOPMENT_TRACE: ON, OFF or DEBUG")
set_property(CACHE CRATCHIT_DEVELOPMENT_TRACE PROPERTY STRINGS ON OFF DEBUG)
if(CRATCHIT_DEVELOPMENT_TRACE STREQUAL "DEBUG")
  set(development_trace "$<CONFIG:Debug>")
elseif(CRATCHIT_DEVELOPMENT_TRACE)
  set(development_trace 1)
else()
  set(development_trace 0)
endif()
target_compile_definitions(cratchit_core PUBLIC CRATCHIT_DEVELOPMENT_TRACE=${development_trace})

# Thin executable on top of the core library
add_executable(cratchit_ar
  src/main.cpp
)
target_link_libraries(cratchit_ar PRIVATE cratchit_core cratchit_warnings)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(test)
endif()
