#!/usr/bin/env python3
"""Build a Jekyll static site from the root of the current git repo."""

import argparse
import shutil
import subprocess
import sys
from pathlib import Path


def git_repo_root() -> Path:
    result = subprocess.run(
        ["git", "rev-parse", "--show-toplevel"],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        sys.exit(f"Not inside a git repo: {result.stderr.strip()}")
    return Path(result.stdout.strip())


def find_jekyll() -> str:
    jekyll = shutil.which("jekyll")
    if jekyll is None:
        sys.exit(
            "jekyll not found on PATH. Make sure chruby has selected the ruby "
            "version Jekyll was installed under (e.g. `chruby 3.4.1`) before "
            "running this script."
        )
    return jekyll


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Build a Jekyll static site from the root of the current git repo."
    )
    parser.add_argument(
        "output_path", type=Path, help="Path to write the generated static site to"
    )
    parser.add_argument(
        "--config",
        type=Path,
        default=None,
        help="Optional Jekyll _config.yml to use instead of Jekyll's defaults",
    )
    args = parser.parse_args()

    repo_root = git_repo_root()
    output_path = args.output_path.resolve()
    jekyll = find_jekyll()

    command = [
        jekyll,
        "build",
        "--source",
        str(repo_root),
        "--destination",
        str(output_path),
    ]
    if args.config is not None:
        command += ["--config", str(args.config.resolve())]

    subprocess.run(command, check=True)
    print(f"Site generated at {output_path}")


if __name__ == "__main__":
    main()
