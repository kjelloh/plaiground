#!/usr/bin/env python3
"""Batch-run eml_to_chime over every .eml file in a folder.

Failures are logged and skipped so one bad eml doesn't stop the run.
"""

import sys
from pathlib import Path

from eml_to_chime import eml_file_to_chime

# When stdout and stderr are both redirected to the same file (e.g.
# `> output.log 2>&1`), stdout is block-buffered while stderr isn't, so
# FAIL lines (stderr) can land mid-way through an unflushed stdout line.
# Force line buffering so writes interleave in the order they're printed.
sys.stdout.reconfigure(line_buffering=True)
sys.stderr.reconfigure(line_buffering=True)


def to_dir_path(path_str: str) -> Path:
    dir_path = Path(path_str)
    if not dir_path.is_dir():
        raise NotADirectoryError(f"Not a directory: {dir_path}")

    return dir_path


def main() -> None:
    if len(sys.argv) != 2:
        sys.exit(f"Usage: {sys.argv[0]} <path-to-eml-folder>")

    try:
        eml_dir = to_dir_path(sys.argv[1])
    except Exception as e:
        sys.exit(f"Exception: {e}")

    eml_paths = sorted(eml_dir.glob("*.eml"))
    if not eml_paths:
        sys.exit(f"No .eml files found in {eml_dir}")

    ok_count = 0
    fail_count = 0

    for eml_path in eml_paths:
        try:
            eml_file_to_chime(eml_path)
            print(f"OK: {eml_path.name}")
        except Exception as e:
            fail_count += 1
            print(f"FAIL: {eml_path.name}", file=sys.stderr)
            print(f"    └── {type(e).__name__}: {e}", file=sys.stderr)
        else:
            ok_count += 1

    print(f"\n{ok_count} ok, {fail_count} failed, {len(eml_paths)} total")


if __name__ == "__main__":
    main()
