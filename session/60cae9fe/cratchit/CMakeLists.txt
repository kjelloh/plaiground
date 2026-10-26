cmake_minimum_required(VERSION 3.25)

project(cratchit VERSION 0.1.0 LANGUAGES CXX)

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

# Likewise the boxes of SKV's VAT return that accounts report in
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS resources/momsrapport/bas_to_momsrapport.csv)
file(READ resources/momsrapport/bas_to_momsrapport.csv account_boxes_hex HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," account_boxes_bytes "${account_boxes_hex}")
configure_file(src/core/skv/vat_return/account_boxes.cpp.in generated/account_boxes.cpp @ONLY)

# All application logic lives in the core library
add_library(cratchit_core
  src/core/amount.cpp
  src/core/bas.cpp
  src/core/books/books.cpp
  src/core/books/columns.cpp
  src/core/books/entry.cpp
  src/core/books/hint.cpp
  src/core/books/import.cpp
  src/core/books/match.cpp
  src/core/books/report.cpp
  src/core/books/statements.cpp
  src/core/charset/cp437.cpp
  src/core/charset/detect.cpp
  src/core/charset/utf8.cpp
  src/core/csv.cpp
  src/core/data_folder.cpp
  src/core/date.cpp
  src/core/help.cpp
  src/core/item_list.cpp
  src/core/key_input.cpp
  src/core/log.cpp
  src/core/log_view.cpp
  src/core/manual.cpp
  src/core/pager.cpp
  src/core/path_editor.cpp
  src/core/path_value.cpp
  src/core/project.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/generated/manual_text.cpp
  src/core/repl.cpp
  src/core/sie/check.cpp
  src/core/sie/document.cpp
  src/core/sie/file.cpp
  src/core/sie/lexer.cpp
  src/core/skv/vat_return/box.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/generated/account_boxes.cpp
  src/core/skv/vat_return/document.cpp
  src/core/statement/statement.cpp
  src/core/terminal.cpp
  src/core/terminal_encoding.cpp
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
add_executable(cratchit
  src/main.cpp
)
target_link_libraries(cratchit PRIVATE cratchit_core cratchit_warnings)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(test)
endif()
