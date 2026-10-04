import hashlib
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

SITE_REPO = Path(__file__).parent.parent / "site_repo"
EMLS_TO_DOMAIN = Path(__file__).parent / "emls_to_domain.py"


def eml(subject: str, date: str, body: str) -> str:
    return (
        f"Subject: {subject}\r\n"
        "From: foo@bar.se\r\n"
        f"Date: {date}\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "\r\n"
        f"{body}\r\n"
    )


OLD = "Fri, 11 Sep 2026 12:00:00 +0000"
NEW = "Sat, 12 Sep 2026 09:00:00 +0000"


@pytest.fixture
def repo(tmp_path):
    """A target repo with the site_repo scaffolding tools."""
    repo = tmp_path / "repo"
    repo.mkdir()
    shutil.copy2(SITE_REPO / "init_new.py", repo / "init_new.py")
    shutil.copy2(SITE_REPO / "update_index.py", repo / "update_index.py")
    return repo


@pytest.fixture
def eml_dir(tmp_path):
    eml_dir = tmp_path / "emls"
    eml_dir.mkdir()
    return eml_dir


@pytest.fixture(params=["mail", "note"])
def domain(request):
    return request.param


def write(eml_dir: Path, name: str, content: str) -> None:
    (eml_dir / name).write_text(content, encoding="utf-8")


def run(eml_dir, repo, *extra):
    return subprocess.run(
        [sys.executable, str(EMLS_TO_DOMAIN), str(eml_dir), str(repo), *extra],
        capture_output=True,
        text=True,
    )


def short_hash(subject: str) -> str:
    return hashlib.md5(subject.encode("utf-8")).hexdigest()[:8]


def test_imports_all_emls_and_writes_index(eml_dir, repo, domain):
    write(eml_dir, "a.eml", eml("A", OLD, "alpha"))
    write(eml_dir, "b.eml", eml("B", OLD, "beta"))

    result = run(eml_dir, repo, "--domain", domain)

    assert result.returncode == 0, result.stderr
    assert f"2 mail files -> {domain}: 2 added, 0 updated, 0 skipped (superseded), 0 failed" in result.stdout
    for subject in ("A", "B"):
        assert (repo / domain / short_hash(subject) / f"{domain}.md").is_file()
    index = (repo / domain / "index.md").read_text(encoding="utf-8")
    assert f"* [A]({short_hash('A')}/{domain}.md)" in index
    assert f"* [B]({short_hash('B')}/{domain}.md)" in index


def test_exclude_md_in_eml_dir_has_no_effect(eml_dir, repo, domain):
    write(eml_dir, "a.eml", eml("A", OLD, "alpha"))
    (eml_dir / "exclude.md").write_text(f"* [A]({short_hash('A')}/{domain}.md)\n", encoding="utf-8")

    result = run(eml_dir, repo, "--domain", domain)

    assert result.returncode == 0, result.stderr
    assert (repo / domain / short_hash("A") / f"{domain}.md").is_file()


def test_rerun_skips_everything_and_leaves_index_alone(eml_dir, repo, domain):
    write(eml_dir, "a.eml", eml("A", OLD, "alpha"))
    run(eml_dir, repo, "--domain", domain)
    index = repo / domain / "index.md"
    index.write_text("hand edited\n", encoding="utf-8")

    result = run(eml_dir, repo, "--domain", domain)

    assert "0 added, 0 updated, 1 skipped (superseded), 0 failed" in result.stdout
    assert index.read_text(encoding="utf-8") == "hand edited\n"


def test_newer_mail_in_later_run_updates_entry(eml_dir, repo, domain):
    write(eml_dir, "a.eml", eml("A", OLD, "alpha"))
    run(eml_dir, repo, "--domain", domain)
    write(eml_dir, "a 2.eml", eml("A", NEW, "alpha revised"))

    result = run(eml_dir, repo, "--domain", domain)

    assert "UPDATE: a 2.eml" in result.stdout
    assert "SKIP: a.eml" in result.stdout
    assert "0 added, 1 updated, 1 skipped (superseded), 0 failed" in result.stdout
    md = (repo / domain / short_hash("A") / f"{domain}.md").read_text(encoding="utf-8")
    assert "alpha revised" in md


def test_same_subject_in_one_run_keeps_newest(eml_dir, repo, domain):
    # Sorted order processes "a 2.eml" (newer) before "a.eml" (older).
    write(eml_dir, "a.eml", eml("A", OLD, "old body"))
    write(eml_dir, "a 2.eml", eml("A", NEW, "new body"))

    result = run(eml_dir, repo, "--domain", domain)

    assert "1 added, 0 updated, 1 skipped (superseded), 0 failed" in result.stdout
    txt = (repo / domain / short_hash("A") / f"{domain}.txt").read_text(encoding="utf-8-sig")
    assert "new body" in txt


def test_failing_eml_is_reported_and_others_still_imported(eml_dir, repo, domain):
    write(eml_dir, "a.eml", eml("A", OLD, "alpha"))
    write(
        eml_dir,
        "bad.eml",
        "Subject: Bad\r\nContent-Type: application/octet-stream\r\n"
        "Content-Transfer-Encoding: base64\r\n\r\nAAAA\r\n",
    )

    result = run(eml_dir, repo, "--domain", domain)

    assert result.returncode == 0
    assert "FAIL: bad.eml" in result.stderr
    assert "1 added, 0 updated, 0 skipped (superseded), 1 failed" in result.stdout
    assert (repo / domain / short_hash("A") / f"{domain}.md").is_file()


def test_domain_is_required(eml_dir, repo):
    write(eml_dir, "a.eml", eml("A", OLD, "alpha"))

    result = run(eml_dir, repo)

    assert result.returncode != 0
    assert "--domain" in result.stderr
    assert not [p for p in repo.iterdir() if p.is_dir()]


def test_missing_init_new_is_reported(eml_dir, tmp_path, domain):
    write(eml_dir, "a.eml", eml("A", OLD, "alpha"))
    empty_repo = tmp_path / "empty"
    empty_repo.mkdir()

    result = run(eml_dir, empty_repo, "--domain", domain)

    assert result.returncode != 0
    assert "init_new.py not found" in result.stderr
