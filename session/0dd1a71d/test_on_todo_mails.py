#!/usr/bin/env python3
"""Turn my exported todo-mails into a static web site of chimes.

    1. eml_to_domain:    all .eml files -> site_repo/mail
    2. domain_to_domain: mail entries not in ignore.md -> site_repo/chime
    3. to_site:          site_repo -> public_html, served for preview

Only files reachable by links from site_repo/index.md end up on the site.
It links to chime/index.md but not to mail/, so 'mail' stays off the site.

Every step is incremental: re-running adds new mails and new chimes, and
never replaces an entry already in chime (so edits made there are safe).
"""

import subprocess
import sys
from pathlib import Path

SESSION = Path(__file__).resolve().parent
EML_DIR = Path.home() / "Downloads" / "mail_export" / "eml"
SITE_REPO = SESSION / "site_repo"
IGNORE = SESSION / "ignore.md"
PICK = SESSION / "pick.md"

EMLS_TO_DOMAIN = SESSION / "eml_to_domain" / "emls_to_domain.py"
DOMAIN_TO_DOMAIN = SESSION / "domain_to_domain" / "domain_to_domain.py"
TO_SITE = SESSION / "to_site" / "to_site.py"


def run(*args, **kwargs) -> None:
    print(f"\n$ {' '.join(str(a) for a in args)}", flush=True)
    subprocess.run([sys.executable, *map(str, args)], check=True, **kwargs)


def main() -> None:
    if not EML_DIR.is_dir():
        sys.exit(f"Not a directory: {EML_DIR}")

    run(EMLS_TO_DOMAIN, EML_DIR, SITE_REPO, "--domain", "mail")

    if not (SITE_REPO / "chime").is_dir():
        run(DOMAIN_TO_DOMAIN, "init", SITE_REPO, "mail", "chime")

    with PICK.open("w", encoding="utf-8") as pick:
        run(DOMAIN_TO_DOMAIN, "diff", SITE_REPO, "mail", "chime", "--ignore", IGNORE, stdout=pick)
    if PICK.read_text(encoding="utf-8").strip():
        run(DOMAIN_TO_DOMAIN, "add", SITE_REPO, "mail", "chime", "--pick", PICK)
    else:
        print("Nothing new to add to chime.")

    run(TO_SITE, SITE_REPO)  # serves the site until you press Ctrl-C


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as e:
        sys.exit(e.returncode)
    except KeyboardInterrupt:
        pass
