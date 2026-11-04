# walk_pdf

walk_pdf walks the structure of a PDF file and shows each part it visits as a
*path* (where in the file it is) and *content* (what is there). The main aim is
the text of the PDF, so that it can be turned into plain text or Markdown.

> Status: early scaffolding. Only `--help` works so far. Walking PDF files is
> not implemented yet.

## Usage

```
walk_pdf [options]
```

## Options

| Option         | What it does                  |
|----------------|-------------------------------|
| `-h`, `--help` | Print this manual and exit.   |

Running walk_pdf without arguments prints its name and version.

## Exit codes

| Code | Meaning                         |
|------|---------------------------------|
| 0    | Success.                        |
| 2    | Invalid command line arguments. |

## Building

walk_pdf is built with CMake, and its packages come from Conan 2. It needs C++23.

```sh
conan install . --build=missing -s compiler.cppstd=23 -s build_type=Debug
conan install . --build=missing -s compiler.cppstd=23 -s build_type=Release

cmake --preset conan-debug   && cmake --build --preset conan-debug
cmake --preset conan-release && cmake --build --preset conan-release
```

The programs end up in `build/Debug/walk_pdf` and `build/Release/walk_pdf`.

Warnings are treated as errors. To build once with warnings allowed, configure
with `cmake --compile-no-warning-as-error ...`.
