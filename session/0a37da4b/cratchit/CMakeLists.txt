cmake_minimum_required(VERSION 3.25)

project(cratchit VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The Swedish UI texts are UTF-8 in the source; MSVC must be told so
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

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

# Text transcoding: Unicode, UTF-8, UTF-16, CP437 and detection (from walk_pdf, session 9fe36d28),
# and Transcoder that pairs a decoder with an encoder
add_library(charset
  src/charset/cp437.cpp
  src/charset/decoder.cpp
  src/charset/detect.cpp
  src/charset/transcoder.cpp
  src/charset/utf16.cpp
  src/charset/utf8.cpp
)
target_include_directories(charset PUBLIC src)
target_link_libraries(charset PRIVATE cratchit_warnings)

# The terminal UI library pyroceph, namespace pyroceph::tui. Knows nothing of
# cratchit, so that it can become a library of its own.
add_library(pyroceph
  src/pyroceph/tui/error_state.cpp
  src/pyroceph/tui/input_buffer.cpp
  src/pyroceph/tui/key_parser.cpp
  src/pyroceph/tui/key_reader.cpp
  src/pyroceph/tui/prompt_loop.cpp
  src/pyroceph/tui/raw_mode.cpp
  src/pyroceph/tui/state.cpp
)
target_include_directories(pyroceph PUBLIC src)
target_link_libraries(pyroceph PUBLIC charset PRIVATE cratchit_warnings)

# Application logic (from take 3, session 60cae9fe)
add_library(cratchit_core
  src/core/dot_folder.cpp
  src/core/log.cpp
  src/core/path_value.cpp
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

# cratchit's own UI states, built on pyroceph::tui::State. A library of its own
# so that the tests can reach the states.
add_library(cratchit_ui
  src/ui/root_menu.cpp
  src/ui/test_menu.cpp
  src/ui/workspace_menu.cpp
)
target_include_directories(cratchit_ui PUBLIC src)
target_link_libraries(cratchit_ui PUBLIC pyroceph PRIVATE cratchit_warnings)

add_executable(cratchit
  src/main.cpp
)
target_link_libraries(cratchit PRIVATE cratchit_core cratchit_ui pyroceph cratchit_warnings)

# The framework comes with the program: a .cratchit folder next to it, which
# cratchit only reads. resources/ holds its sources; framework.txt gets the
# project version. It is put together in the build folder at configure time.
set(framework_folder ${CMAKE_CURRENT_BINARY_DIR}/framework/.cratchit)
configure_file(resources/framework.txt.in ${framework_folder}/framework.txt @ONLY)

# Copies the framework next to the built program on every build, so that a
# development build finds it just like an installed one
add_custom_target(cratchit_framework ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${framework_folder} $<TARGET_FILE_DIR:cratchit>/.cratchit
  COMMENT "Copying the framework next to cratchit"
)
add_dependencies(cratchit_framework cratchit)

# Installs the program and its framework into <prefix>/bin (e.g. ~/.local/bin):
# bin/cratchit and bin/.cratchit/. The libraries are static and built into the program.
include(GNUInstallDirs)
install(TARGETS cratchit RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY ${framework_folder} DESTINATION ${CMAKE_INSTALL_BINDIR})

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(test)
endif()
