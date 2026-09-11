#!/usr/bin/env python3
"""
init_python_tool_chain: Ensure a project-local virtualenv exists at
.venv/ and that it has the packages this project's tooling needs
(currently just pytest) installed.

Safe to re-run: creates the venv only if missing, and pip-installs
are no-ops when already satisfied.
"""

import subprocess
import sys
import venv
from pathlib import Path

VENV_DIR = Path(__file__).parent / ".venv"
PACKAGES = ["pytest"]


def venv_python(venv_dir: Path) -> Path:
    if sys.platform == "win32":
        return venv_dir / "Scripts" / "python.exe"
    return venv_dir / "bin" / "python"


def ensure_venv(venv_dir: Path) -> None:
    if venv_python(venv_dir).exists():
        print(f"venv    -> {venv_dir} (already exists)")
        return
    venv.EnvBuilder(with_pip=True).create(venv_dir)
    print(f"venv    -> {venv_dir} (created)")


def ensure_packages(venv_dir: Path, packages: list[str]) -> None:
    subprocess.run(
        [str(venv_python(venv_dir)), "-m", "pip", "install", "-q", *packages],
        check=True,
    )
    print(f"packages -> {', '.join(packages)} (installed)")


def activate_hint(venv_dir: Path) -> str:
    if sys.platform == "win32":
        return f"{venv_dir}\\Scripts\\activate"
    return f"source {venv_dir}/bin/activate"


def main() -> None:
    ensure_venv(VENV_DIR)
    ensure_packages(VENV_DIR, PACKAGES)
    print(
        "\nThis venv is not active in your shell (this script only set it up).\n"
        f"Run this yourself to activate it:\n\n    {activate_hint(VENV_DIR)}\n"
    )


if __name__ == "__main__":
    main()
