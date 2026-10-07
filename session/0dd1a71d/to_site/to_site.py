#!/usr/bin/env python3
"""
to_site.py: Stage only the files reachable from index.md into a temporary
'_staged_site' folder, build that staged folder as a Jekyll site (via
to_jekyll_site.py), then publish and serve the built site (via
publish_site.py) for browser preview. The Jekyll toolchain/output
('.jekyll') is kept in the local site root, alongside '_staged_site', not
inside it.
"""

import shutil
import sys
from pathlib import Path

from stage_site import stage_site, STAGED_DIR_NAME
from to_jekyll_site import build
from publish_site import publish_site, serve, DEFAULT_TARGET_NAME, DEFAULT_PORT


def main():
    site_root = (Path(sys.argv[1]) if len(sys.argv) > 1 else Path.cwd()).resolve()
    target_name = sys.argv[2] if len(sys.argv) > 2 else DEFAULT_TARGET_NAME
    port = int(sys.argv[3]) if len(sys.argv) > 3 else DEFAULT_PORT

    staged_dir = site_root / STAGED_DIR_NAME
    stage_site(site_root, staged_dir)

    # _config.yml drives the Jekyll build itself; it isn't reachable via
    # markdown links from index.md, so stage it explicitly.
    config_file = site_root / "_config.yml"
    if config_file.exists():
        shutil.copy2(config_file, staged_dir / "_config.yml")

    print(f"Staged reachable files into '{staged_dir}'.")

    jekyll_dir = site_root / ".jekyll"
    site_dir = build(staged_dir, jekyll_dir)
    if site_dir is None:
        sys.exit(1)

    published_dir = site_root / target_name
    publish_site(site_dir, published_dir)
    print(f"Published '{site_dir}' to '{published_dir}'.")

    sys.exit(serve(published_dir, port))


if __name__ == "__main__":
    main()
