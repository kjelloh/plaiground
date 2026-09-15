#!/usr/bin/env python3
"""
to_site.py: Run from within a git repo configured for GitHub Pages — the
current working directory is used as the 'source'. Ensures a '.jekyll'
folder there holds the Jekyll toolchain and build output, then builds and
serves the site.
"""

import subprocess
import sys
from pathlib import Path
import shutil


#import init_jekyll_tool_chain as tool_chain

SCRIPT_DIR = Path(__file__).resolve().parent
JEKYLL_DIR = SCRIPT_DIR / ".jekyll"

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

def ensure_jekyll_folder():
    JEKYLL_DIR.mkdir(exist_ok=True)

GEMFILE_CONTENT = '''source "https://rubygems.org"

gem "github-pages", group: :jekyll_plugins
'''

def ensure_gemfile():
    gemfile = JEKYLL_DIR / "Gemfile"
    if not gemfile.exists() or gemfile.read_text() != GEMFILE_CONTENT:
        gemfile.write_text(GEMFILE_CONTENT)

def ensure_jekyll_tool_chain():
    """Idempotently ensure '.jekyll' is a ready Jekyll build environment. Returns True on success."""
    if not ensure_ruby_env():
        return False
    ensure_jekyll_folder()
    ensure_gemfile()
    return ensure_bundle_installed()

def ensure_bundle_installed():
    result = subprocess.run(["bundle", "install"], cwd=JEKYLL_DIR)
    return result.returncode == 0

def build_site(source, site_dir):
    command = [
        "bundle", "exec", "jekyll", "build",
        "--source", str(source),
        "--destination", str(site_dir),
    ]
    result = subprocess.run(command, cwd=JEKYLL_DIR)
    return result.returncode == 0


def serve_site(source, site_dir):
    command = [
        "bundle", "exec", "jekyll", "serve",
        "--source", str(source),
        "--destination", str(site_dir),
        "--skip-initial-build",
    ]
    result = subprocess.run(command, cwd=JEKYLL_DIR)
    return result.returncode == 0


def main():
    source = Path.cwd()

    config_file = source / "_config.yml"
    if not config_file.exists():
        print(f"'{config_file}' not found. Run this from a Jekyll/GitHub Pages repo's top folder.")
        sys.exit(1)

    JEKYLL_DIR = source / ".jekyll"
    site_dir = JEKYLL_DIR / "_site"

    if not ensure_jekyll_tool_chain():
        print("Failed to prepare the '.jekyll' tool chain.")
        sys.exit(1)

    if not build_site(source, site_dir):
        print("Jekyll build failed.")
        sys.exit(1)

    print(f"Site built to '{site_dir}'. Starting local server (press Ctrl-C to stop)...")
    sys.exit(0 if serve_site(source, site_dir) else 1)


if __name__ == "__main__":
    main()
