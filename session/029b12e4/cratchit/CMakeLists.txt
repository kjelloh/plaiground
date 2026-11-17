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

# Text transcoding: Unicode, UTF-8, UTF-16, CP437, WinAnsi, MacRoman, Adobe Standard and detection
# (from walk_pdf, session 9fe36d28),
# and Transcoder that pairs a decoder with an encoder
add_library(charset
  src/charset/adobe_standard.cpp
  src/charset/cp437.cpp
  src/charset/decoder.cpp
  src/charset/detect.cpp
  src/charset/macroman.cpp
  src/charset/transcoder.cpp
  src/charset/utf16.cpp
  src/charset/utf8.cpp
  src/charset/winansi.cpp
)
target_include_directories(charset PUBLIC src)
target_link_libraries(charset PRIVATE cratchit_warnings)

find_package(ZLIB REQUIRED)

# walk_pdf (session 9fe36d28): the text of a PDF, namespace walk_pdf. Its own
# sources unchanged in src/walk_pdf, its main.cpp as cli.cpp (cratchit --walk_pdf)
# and its manual as docs/walk_pdf.md; the charsets it needs are in charset.
# See src/walk_pdf/walk_pdf.hpp.
# docs/walk_pdf.md into the program as bytes, for cratchit --walk_pdf --help;
# CMake re-runs when it changes
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS docs/walk_pdf.md)
file(READ docs/walk_pdf.md walk_pdf_manual_hex HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," manual_bytes "${walk_pdf_manual_hex}")
configure_file(src/walk_pdf/manual_text.cpp.in generated/walk_pdf_manual_text.cpp @ONLY)

add_library(walk_pdf
  src/walk_pdf/cli.cpp
  src/walk_pdf/cmap.cpp
  src/walk_pdf/crypto/md5.cpp
  src/walk_pdf/crypto/rc4.cpp
  src/walk_pdf/document.cpp
  src/walk_pdf/filters.cpp
  src/walk_pdf/font.cpp
  src/walk_pdf/glyph_names.cpp
  src/walk_pdf/path.cpp
  src/walk_pdf/peek.cpp
  src/walk_pdf/plain_text.cpp
  src/walk_pdf/scanner.cpp
  src/walk_pdf/security.cpp
  src/walk_pdf/syntax.cpp
  src/walk_pdf/text_string.cpp
  src/walk_pdf/walk_pdf.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/generated/walk_pdf_manual_text.cpp
)
target_include_directories(walk_pdf PUBLIC src/walk_pdf)
target_link_libraries(walk_pdf PUBLIC charset PRIVATE ZLIB::ZLIB cratchit_warnings)

# The terminal UI library pyroceph, namespace pyroceph::tui. Knows nothing of
# cratchit, so that it can become a library of its own.
add_library(pyroceph
  src/pyroceph/tui/error_state.cpp
  src/pyroceph/tui/input_buffer.cpp
  src/pyroceph/tui/key_parser.cpp
  src/pyroceph/tui/key_reader.cpp
  src/pyroceph/tui/pager.cpp
  src/pyroceph/tui/path_completion.cpp
  src/pyroceph/tui/prompt_loop.cpp
  src/pyroceph/tui/quit_state.cpp
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
  src/core/event_draft.cpp
  src/core/fiscal_year.cpp
  src/core/inventory.cpp
  src/core/log.cpp
  src/core/manual.cpp
  src/core/path_value.cpp
  src/core/pdf_documents.cpp
  src/core/project.cpp
  src/core/sie/check.cpp
  src/core/sie/document.cpp
  src/core/sie/file.cpp
  src/core/sie/lexer.cpp
  src/core/sie/writer.cpp
  src/core/sie_export.cpp
  src/core/sie_import.cpp
  src/core/sie_project.cpp
  src/core/sha256.cpp
  src/core/sie_summary.cpp
  src/core/statement/statement.cpp
  src/core/statement_drafts.cpp
  src/core/terminal_encoding.cpp
  src/core/version.cpp
)
target_include_directories(cratchit_core PUBLIC src)
target_link_libraries(cratchit_core PUBLIC charset PRIVATE walk_pdf cratchit_warnings)
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
  src/ui/booked_menu.cpp
  src/ui/design_insufficiency.cpp
  src/ui/details_project_menu.cpp
  src/ui/draft_menu.cpp
  src/ui/fiscal_year_menu.cpp
  src/ui/forget_project_menu.cpp
  src/ui/project_menu.cpp
  src/ui/root_menu.cpp
  src/ui/sie_import_menu.cpp
  src/ui/sie_project_menu.cpp
  src/ui/voucher_menu.cpp
  src/ui/start.cpp
  src/ui/test_menu.cpp
  src/ui/workspace_menu.cpp
  src/ui/year_flow_menu.cpp
  src/ui/year_view.cpp
)
target_include_directories(cratchit_ui PUBLIC src)
target_link_libraries(cratchit_ui PUBLIC pyroceph cratchit_core PRIVATE cratchit_warnings)

# The time and type of the build, shown when cratchit starts. The target runs on
# every build and writes build_stamp.cpp anew, so only that file is compiled and
# cratchit relinked. The file is written once at configure time too, so that it
# exists before the first build.
set(build_stamp_file ${CMAKE_CURRENT_BINARY_DIR}/generated/build_stamp.cpp)
if(NOT EXISTS ${build_stamp_file})
  execute_process(COMMAND ${CMAKE_COMMAND} -DOUT=${build_stamp_file} -DCONFIG=unknown
                  -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/build_stamp.cmake)
endif()
add_custom_target(cratchit_build_stamp
  COMMAND ${CMAKE_COMMAND} -DOUT=${build_stamp_file} -DCONFIG=$<CONFIG>
          -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/build_stamp.cmake
  BYPRODUCTS ${build_stamp_file}
  COMMENT "Stamping the build time into cratchit"
)

add_executable(cratchit
  src/main.cpp
  ${build_stamp_file}
)
add_dependencies(cratchit cratchit_build_stamp)
target_link_libraries(cratchit PRIVATE cratchit_core cratchit_ui pyroceph walk_pdf cratchit_warnings)

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

# Installs the program and its framework into <prefix>/bin, by default for the whole
# computer (CMAKE_INSTALL_PREFIX: /usr/local, on Windows C:/Program Files/cratchit):
# bin/cratchit and bin/.cratchit/. The libraries are static and built into the program.
include(GNUInstallDirs)
install(TARGETS cratchit RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY ${framework_folder} DESTINATION ${CMAKE_INSTALL_BINDIR})

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(test)
endif()
