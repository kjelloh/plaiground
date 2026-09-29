#!/usr/bin/env python3
"""
to_site.py: Stage only the files reachable from index.md into a temporary
'_staged_site' folder, then build and serve that staged folder as a Jekyll
site (via to_jekyll_site.py). The Jekyll toolchain/output ('.jekyll') is
kept in the local site root, alongside '_staged_site', not inside it.
"""

import shutil
import sys
from pathlib import Path

from stage_site import stage_site, STAGED_DIR_NAME
from to_jekyll_site import build_and_serve


def main():
    site_root = (Path(sys.argv[1]) if len(sys.argv) > 1 else Path.cwd()).resolve()
    staged_dir = site_root / STAGED_DIR_NAME

    stage_site(site_root, staged_dir)

    # _config.yml drives the Jekyll build itself; it isn't reachable via
    # markdown links from index.md, so stage it explicitly.
    config_file = site_root / "_config.yml"
    if config_file.exists():
        shutil.copy2(config_file, staged_dir / "_config.yml")

    print(f"Staged reachable files into '{staged_dir}'.")
    jekyll_dir = site_root / ".jekyll"
    sys.exit(build_and_serve(staged_dir, jekyll_dir))


if __name__ == "__main__":
    main()
