#!/usr/bin/env python3
"""
publish_site.py: Copy a built Jekyll '_site' output into a target-named
folder (e.g. 'public_html', matching a web hotel's expected site-root
folder name), then serve that folder locally so it can be previewed in a
browser exactly as it would be deployed.
"""

import shutil
import sys
from functools import partial
from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
from pathlib import Path

DEFAULT_TARGET_NAME = "public_html"
DEFAULT_PORT = 8000


def publish_site(site_dir: Path, published_dir: Path) -> None:
    """Mirror a built Jekyll 'site_dir' into 'published_dir'."""
    if published_dir.exists():
        shutil.rmtree(published_dir)
    shutil.copytree(site_dir, published_dir)
    apply_target_tweaks(published_dir)


def apply_target_tweaks(published_dir: Path) -> None:
    """Hook for target-specific post-processing (e.g. a web hotel's own
    quirks, image format conversion). No-op for now."""
    pass


def serve(published_dir: Path, port: int) -> int:
    handler = partial(SimpleHTTPRequestHandler, directory=str(published_dir))
    with ThreadingHTTPServer(("127.0.0.1", port), handler) as httpd:
        print(f"Serving '{published_dir}' at http://127.0.0.1:{port}/ (press Ctrl-C to stop)...")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            pass
    return 0


def main():
    site_root = (Path(sys.argv[1]) if len(sys.argv) > 1 else Path.cwd()).resolve()
    target_name = sys.argv[2] if len(sys.argv) > 2 else DEFAULT_TARGET_NAME
    port = int(sys.argv[3]) if len(sys.argv) > 3 else DEFAULT_PORT

    site_dir = site_root / ".jekyll" / "_site"
    if not site_dir.exists():
        print(f"'{site_dir}' not found. Build the site first (e.g. run to_site.py).")
        sys.exit(1)

    published_dir = site_root / target_name
    publish_site(site_dir, published_dir)
    print(f"Published '{site_dir}' to '{published_dir}'.")

    sys.exit(serve(published_dir, port))


if __name__ == "__main__":
    main()
