#!/usr/bin/env python3
"""
to_jekyll_site.py: Build and serve a Jekyll site from a given source
directory. Ensures a '.jekyll' folder inside that source holds the Jekyll
toolchain and build output, then builds and serves the site.
"""

import subprocess
import sys
from pathlib import Path
import shutil


def ensure_ruby_env():
    """Verify a chruby-managed ruby/bundle are on PATH (not system Ruby)."""
    ruby = shutil.which("ruby")
    bundle = shutil.which("bundle")
    if not ruby or not bundle:
        print("ruby/bundle not found on PATH.")
        print("In this shell, source chruby and select a ruby version, e.g.:")
        print("  source /opt/homebrew/opt/chruby/share/chruby/chruby.sh")
        print("  chruby ruby-3.4.1")
        return False
    if ".rubies" not in ruby:
        print(f"'{ruby}' looks like the system Ruby, not a chruby-managed one.")
        print("Run 'chruby ruby-3.4.1' (or similar) in this shell first.")
        return False
    return True

def ensure_jekyll_folder(jekyll_dir):
    jekyll_dir.mkdir(exist_ok=True)

GEMFILE_CONTENT = '''source "https://rubygems.org"

gem "github-pages", group: :jekyll_plugins
'''

def ensure_gemfile(jekyll_dir):
    gemfile = jekyll_dir / "Gemfile"
    if not gemfile.exists() or gemfile.read_text() != GEMFILE_CONTENT:
        gemfile.write_text(GEMFILE_CONTENT)

def ensure_jekyll_tool_chain(jekyll_dir):
    """Idempotently ensure 'jekyll_dir' is a ready Jekyll build environment. Returns True on success."""
    if not ensure_ruby_env():
        return False
    ensure_jekyll_folder(jekyll_dir)
    ensure_gemfile(jekyll_dir)
    return ensure_bundle_installed(jekyll_dir)

def ensure_bundle_installed(jekyll_dir):
    result = subprocess.run(["bundle", "install"], cwd=jekyll_dir)
    return result.returncode == 0

def build_site(source, site_dir, jekyll_dir):
    command = [
        "bundle", "exec", "jekyll", "build",
        "--source", str(source),
        "--destination", str(site_dir),
    ]
    result = subprocess.run(command, cwd=jekyll_dir)
    return result.returncode == 0


def serve_site(source, site_dir, jekyll_dir):
    command = [
        "bundle", "exec", "jekyll", "serve",
        "--source", str(source),
        "--destination", str(site_dir),
        "--skip-initial-build",
    ]
    result = subprocess.run(command, cwd=jekyll_dir)
    return result.returncode == 0


def build_and_serve(source: Path, jekyll_dir: Path = None) -> int:
    """Build then serve a Jekyll site from 'source'. The Jekyll toolchain and
    build output live under 'jekyll_dir' (defaults to 'source/.jekyll').
    Returns a process exit code."""
    source = source.resolve()
    jekyll_dir = (jekyll_dir if jekyll_dir is not None else source / ".jekyll").resolve()

    config_file = source / "_config.yml"
    if not config_file.exists():
        print(f"'{config_file}' not found. Run this on a Jekyll/GitHub Pages source folder.")
        return 1

    site_dir = jekyll_dir / "_site"

    if not ensure_jekyll_tool_chain(jekyll_dir):
        print(f"Failed to prepare the '{jekyll_dir}' tool chain.")
        return 1

    if not build_site(source, site_dir, jekyll_dir):
        print("Jekyll build failed.")
        return 1

    print(f"Site built to '{site_dir}'. Starting local server (press Ctrl-C to stop)...")
    return 0 if serve_site(source, site_dir, jekyll_dir) else 1


def main():
    source = Path(sys.argv[1]) if len(sys.argv) > 1 else Path.cwd()
    sys.exit(build_and_serve(source))


if __name__ == "__main__":
    main()
