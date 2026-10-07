#!/usr/bin/env python3
"""
to_jekyll_site.py: Build a Jekyll site from a given source directory.
Ensures a '.jekyll' folder inside that source holds the Jekyll toolchain
and build output. Does not serve the site — see publish_site.py for that.

Jekyll's own output is shown as it runs, except for a fatal error: its
message (which may quote a long stretch of the failing page) and Ruby
backtrace are replaced by a short summary of the failing file and error.
"""

import re
import subprocess
import sys
from pathlib import Path
import shutil

# "  Liquid Exception: <message> in <file>" — Jekyll's log line for the
# failing page; <message> may span several lines.
EXCEPTION_START_RE = re.compile(r'^\s*(?P<topic>[A-Za-z ]*(?:Exception|Error)):\s*(?P<text>.*)$')
EXCEPTION_FILE_RE = re.compile(r'^(?P<message>.*) in (?P<file>[^\s].*?)\s*$', re.S)
# "jekyll 3.10.0 | Error:  <message>" — the start of the fatal error dump.
FATAL_ERROR_RE = re.compile(r'^jekyll \S+ \| Error:\s*(?P<text>.*)$')
# Jekyll colours its warning/error lines, e.g. "\x1b[31m ... \x1b[0m".
ANSI_COLOUR_RE = re.compile(r'\x1b\[[0-9;]*m')
QUOTE_MAX = 40
MESSAGE_MAX = 200

LIQUID_FIX = (
    "The page contains '{{' or '{%' that Liquid reads as template syntax. "
    "If it is meant as plain text, wrap it in {% raw %} ... {% endraw %}."
)


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
    process = subprocess.Popen(
        command, cwd=jekyll_dir, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        text=True, encoding="utf-8", errors="replace",
    )
    held: list[str] = []  # from the first error line on, colour codes removed
    for line in process.stdout:
        plain = ANSI_COLOUR_RE.sub("", line)
        if held or EXCEPTION_START_RE.match(plain) or FATAL_ERROR_RE.match(plain):
            held.append(plain)
        else:
            print(line, end="", flush=True)
    if process.wait() != 0:
        print("\n".join(failure_summary(held, source)))
        return False
    print("".join(held), end="")  # an error line that didn't stop the build
    return True


def shorten(message: str) -> str:
    """message on one line, with a long quoted stretch of page text and
    the message as a whole cut short."""
    message = " ".join(message.split())
    message = re.sub(
        r"'(.*)'",  # first to last quote: the quoted text may hold quotes itself
        lambda m: f"'{m[1][:QUOTE_MAX]}…'" if len(m[1]) > QUOTE_MAX else m[0],
        message,
    )
    return message if len(message) <= MESSAGE_MAX else message[:MESSAGE_MAX] + "…"


def failure_summary(held: list[str], source: Path) -> list[str]:
    """A short report of a failed Jekyll build from the output lines held
    back from the first error line on: the failing file (if Jekyll named
    one), the error, and for a Liquid error how to fix it."""
    file = None
    message = None
    exception: list[str] = []
    for line in held:
        if FATAL_ERROR_RE.match(line):
            break
        if exception or EXCEPTION_START_RE.match(line):
            exception.append(line)
    if exception:
        start = EXCEPTION_START_RE.match(exception[0])
        text = "\n".join([start["text"]] + [line.rstrip("\n") for line in exception[1:]])
        match = EXCEPTION_FILE_RE.match(text)
        message, file = (match["message"], match["file"]) if match else (text, None)
    else:
        # No per-page exception line: use the fatal error's message, up to
        # the Ruby backtrace.
        fatal: list[str] = []
        for line in held:
            if fatal and re.search(r'\.rb:\d+:in ', line):
                break
            if fatal or FATAL_ERROR_RE.match(line):
                fatal.append(FATAL_ERROR_RE.sub(r'\g<text>', line))
        message = "".join(fatal) or "(no error message)"

    if file is not None:
        try:
            file = str(Path(file).relative_to(source))
        except ValueError:
            pass

    lines = ["", "Jekyll build failed."]
    if file is not None:
        lines.append(f"  File:  {file}")
    lines.append(f"  Error: {shorten(message)}")
    if "liquid" in message.lower() or (exception and "liquid" in exception[0].lower()):
        lines.append(f"  Fix:   {LIQUID_FIX}")
    return lines


def build(source: Path, jekyll_dir: Path = None) -> Path:
    """Build a Jekyll site from 'source'. The Jekyll toolchain and build
    output live under 'jekyll_dir' (defaults to 'source/.jekyll'). Returns
    the built site_dir on success, or None on failure."""
    source = source.resolve()
    jekyll_dir = (jekyll_dir if jekyll_dir is not None else source / ".jekyll").resolve()

    config_file = source / "_config.yml"
    if not config_file.exists():
        print(f"'{config_file}' not found. Run this on a Jekyll/GitHub Pages source folder.")
        return None

    site_dir = jekyll_dir / "_site"

    if not ensure_jekyll_tool_chain(jekyll_dir):
        print(f"Failed to prepare the '{jekyll_dir}' tool chain.")
        return None

    if not build_site(source, site_dir, jekyll_dir):
        return None  # build_site has reported why

    print(f"Site built to '{site_dir}'.")
    return site_dir


def main():
    source = Path(sys.argv[1]) if len(sys.argv) > 1 else Path.cwd()
    sys.exit(0 if build(source) is not None else 1)


if __name__ == "__main__":
    main()
