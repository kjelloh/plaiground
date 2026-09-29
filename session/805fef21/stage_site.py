#!/usr/bin/env python3
"""
stage_site.py: Copy every file reachable from a root index.md into a
temporary '_staged_site' folder, ready for a Jekyll build (e.g. to_site.py
run with '_staged_site' as its source) that only sees linked-to files.
"""

import shutil
import sys
from pathlib import Path

from reachable import ReachabilityGraph

STAGED_DIR_NAME = "_staged_site"


def stage_site(site_root: Path, staged_dir: Path) -> None:
    root_md = site_root / "index.md"
    graph = ReachabilityGraph(site_root)
    adjacency = graph.build(root_md)

    if staged_dir.exists():
        shutil.rmtree(staged_dir)
    staged_dir.mkdir(parents=True)

    for src in adjacency:
        rel = src.relative_to(site_root)
        dest = staged_dir / rel
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dest)


def main():
    site_root = (Path(sys.argv[1]) if len(sys.argv) > 1 else Path.cwd()).resolve()
    staged_dir = site_root / STAGED_DIR_NAME

    stage_site(site_root, staged_dir)
    print(f"Staged reachable files into '{staged_dir}'.")


if __name__ == "__main__":
    main()
