#!/usr/bin/env python3

import sys
from pathlib import Path

def to_path(path_str: str) -> Path:
    eml_path = Path(path_str)
    if not eml_path.is_file():
        raise FileNotFoundError(f"Not a file: {eml_path}")
    return eml_path

def main() -> None:

  if len(sys.argv) != 2:
      sys.exit(f"Usage: {sys.argv[0]} <path-to-eml-file>")

  try:
      eml_path = to_path(sys.argv[1])
  except FileNotFoundError as e:
      sys.exit(f"Exception: {e}")

if __name__ == "__main__":
    main()