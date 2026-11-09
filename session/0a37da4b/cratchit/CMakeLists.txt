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
  src/pyroceph/tui/path_completion.cpp
  src/pyroceph/tui/prompt_loop.cpp
  src/pyroceph/tui/raw_mode.cpp
  src/pyroceph/tui/state.cpp
  src/pyroceph/tui/terminal_width.cpp
)
target_include_directories(pyroceph PUBLIC src)
target_link_libraries(pyroceph PUBLIC charset PRIVATE cratchit_warnings)

# Application logic (from take 3, session 60cae9fe)
add_library(cratchit_core
  src/core/amount.cpp
  src/core/bas.cpp
  src/core/books/books.cpp
  src/core/books/export.cpp
  src/core/books/import.cpp
  src/core/csv.cpp
  src/core/date.cpp
  src/core/details_project.cpp
  src/core/dot_folder.cpp
  src/core/fiscal_year.cpp
  src/core/log.cpp
  src/core/manual.cpp
  src/core/path_value.cpp
  src/core/project.cpp
  src/core/sie/check.cpp
  src/core/sie/document.cpp
  src/core/sie/file.cpp
  src/core/sie/lexer.cpp
  src/core/sie/writer.cpp
  src/core/sie_export.cpp
  src/core/sie_import.cpp
  src/core/sie_project.cpp
  src/core/terminal_encoding.cpp
  src/core/version.cpp
)
target_include_directories(cratchit_core PUBLIC src)
target_link_libraries(cratchit_core PUBLIC charset PRIVATE cratchit_warnings)
target_compile_definitions(cratchit_core PRIVATE CRATCHIT_VERSION="${PROJECT_VERSION}")
if(WIN32)
  # SHGetKnownFolderPath and CoTaskMemFree, for the Documents folder
  target_link_libraries(cratchit_core PRIVATE shell32 ole32)
endif()

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
  src/ui/add_project_menu.cpp
  src/ui/design_insufficiency.cpp
  src/ui/details_project_menu.cpp
  src/ui/fiscal_year_menu.cpp
  src/ui/forget_project_menu.cpp
  src/ui/project_menu.cpp
  src/ui/root_menu.cpp
  src/ui/sie_import_menu.cpp
  src/ui/sie_project_menu.cpp
  src/ui/test_menu.cpp
  src/ui/workspace_menu.cpp
)
target_include_directories(cratchit_ui PUBLIC src)
target_link_libraries(cratchit_ui PUBLIC pyroceph cratchit_core PRIVATE cratchit_warnings)

add_executable(cratchit
  src/main.cpp
)
target_link_libraries(cratchit PRIVATE cratchit_core cratchit_ui pyroceph cratchit_warnings)

# The framework comes with the program: a .cratchit folder next to it, which
# cratchit only reads. resources/ holds its sources; framework.txt gets the
# project version. It is put together in the build folder at configure time.
set(framework_folder ${CMAKE_CURRENT_BINARY_DIR}/framework/.cratchit)
configure_file(resources/framework.txt.in ${framework_folder}/framework.txt @ONLY)
# The user manual comes with the framework, so that ? can show its help paragraphs
configure_file(docs/anvandarmanual.md ${framework_folder}/anvandarmanual.md COPYONLY)

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
