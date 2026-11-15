#!/usr/bin/env python3
"""Build, test and install cratchit in one go.

    python3 install.py                  # install into ~/.local (program in ~/.local/bin)
    python3 install.py --prefix <dir>   # install somewhere else
    python3 install.py --try            # install into _try/ here and run it there
    python3 install.py --no-tests       # skip the tests

Runs the same steps as README.md, "Build and install", and prints each command
before it runs it. Stops at the first failing step.

Every path is taken from this script's own folder, so it works from any current
folder and keeps working when the cratchit folder is moved. A build folder left
from before a move holds the old full paths, which CMake refuses; the script
then removes it (only generated files) and builds afresh.

--try installs into _try/ next to this script and runs the installed program
with _try/home as its home folder, so your real ~/.cratchit is left alone.
_try/home is kept between tries; delete _try/ to start afresh.
"""

import argparse
import os
import platform
import shlex
import shutil
import subprocess
import sys
from pathlib import Path

# The cratchit folder: where this script is
HERE = Path(__file__).resolve().parent

IS_WINDOWS = platform.system() == "Windows"

# Visual Studio is a multi-config generator: one build folder for all
# configurations, chosen with --config. Elsewhere each build type has its own.
CONFIGURE_PRESET = "conan-default" if IS_WINDOWS else "conan-release"
BUILD_FOLDER = Path("build") if IS_WINDOWS else Path("build") / "Release"
CONFIG_ARGS = ["--config", "Release"] if IS_WINDOWS else []

TRY_FOLDER = HERE / "_try"


def run(step, command, env=None):
    print(f"\n== {step}\n$ {shlex.join(str(part) for part in command)}", flush=True)
    try:
        result = subprocess.run(command, cwd=HERE, env=env)
    except FileNotFoundError:
        sys.exit(f"install.py: '{command[0]}' is not found. See README.md, Requirements.")
    if result.returncode != 0:
        sys.exit(f"install.py: '{step}' failed (exit {result.returncode}).")


def build_folder_is_moved():
    """True when the build folder was made in another place, i.e. the cratchit
    folder was moved or copied. CMake keeps full paths in CMakeCache.txt and
    refuses to configure such a build folder."""
    cache = HERE / BUILD_FOLDER / "CMakeCache.txt"
    if not cache.is_file():
        return False
    for line in cache.read_text(errors="replace").splitlines():
        if line.startswith("CMAKE_CACHEFILE_DIR:"):
            made_in = Path(line.split("=", 1)[1]).resolve()
            here = (HERE / BUILD_FOLDER).resolve()
            return os.path.normcase(str(made_in)) != os.path.normcase(str(here))
    return False


def main():
    parser = argparse.ArgumentParser(description="Build, test and install cratchit.")
    where = parser.add_mutually_exclusive_group()
    where.add_argument("--prefix", type=Path, default=Path.home() / ".local",
                       help="install into PREFIX/bin (default: ~/.local)")
    where.add_argument("--try", dest="try_it", action="store_true",
                       help="install into _try/ next to this script and run it with _try/home as home folder")
    parser.add_argument("--no-tests", action="store_true", help="skip the tests")
    args = parser.parse_args()

    prefix = TRY_FOLDER if args.try_it else args.prefix.expanduser().resolve()
    program_folder = prefix / "bin"

    if build_folder_is_moved():
        # Only generated files are in there, so it is safe to start it afresh
        print(f"The build folder {BUILD_FOLDER} was made in another place (the cratchit folder was moved). "
              "Removing it to build afresh.")
        shutil.rmtree(HERE / BUILD_FOLDER)

    run("1. Fetch dependencies (Conan)",
        ["conan", "install", ".", "--build=missing", "-s", "build_type=Release", "-s", "compiler.cppstd=23"])
    run("2. Configure (CMake)", ["cmake", "--preset", CONFIGURE_PRESET])
    run("3. Build", ["cmake", "--build", "--preset", "conan-release"])
    if args.no_tests:
        print("\n== 4. Tests skipped (--no-tests)")
    else:
        run("4. Test", ["ctest", "--preset", "conan-release"])
    run("5. Install", ["cmake", "--install", str(BUILD_FOLDER), *CONFIG_ARGS, "--prefix", str(prefix)])

    print(f"\nInstalled cratchit and its framework in {program_folder}")

    if args.try_it:
        home = TRY_FOLDER / "home"
        home.mkdir(parents=True, exist_ok=True)
        env = dict(os.environ, HOME=str(home), USERPROFILE=str(home))
        program = program_folder / ("cratchit.exe" if IS_WINDOWS else "cratchit")
        run(f"6. Run the installed cratchit, home folder {home}", [str(program)], env=env)
        return

    paths = [Path(p).resolve() for p in os.environ.get("PATH", "").split(os.pathsep) if p]
    if program_folder not in paths:
        print(f"Note: {program_folder} is not in your PATH. Add it to start cratchit by name.")


if __name__ == "__main__":
    main()
