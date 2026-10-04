import hashlib
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

SITE_REPO = Path(__file__).parent.parent / "site_repo"
DOMAIN_TO_DOMAIN = Path(__file__).parent / "domain_to_domain.py"

SOURCE = "inbox"
TARGET = "todo"


def short_hash(heading: str) -> str:
    return hashlib.md5(heading.encode("utf-8")).hexdigest()[:8]


@pytest.fixture
def repo(tmp_path):
    repo = tmp_path / "repo"
    repo.mkdir()
    shutil.copy2(SITE_REPO / "init_new.py", repo / "init_new.py")
    shutil.copy2(SITE_REPO / "update_index.py", repo / "update_index.py")
    return repo


def make_entry(repo: Path, domain: str, heading: str, body: str = "body", with_txt: bool = True) -> Path:
    """An entry laid out like eml_to_domain writes it."""
    folder = repo / domain / short_hash(heading)
    folder.mkdir(parents=True)
    md = f"# {heading}\n\n2026-09-11T12:00:00+00:00\n\n"
    if with_txt:
        md += f"[Plain text]({domain}.txt)\n\n"
        (folder / f"{domain}.txt").write_text(f"{body}\n", encoding="utf-8")
    md += f"{body}\n\n![pic](pic.png)\n"
    (folder / f"{domain}.md").write_text(md, encoding="utf-8")
    (folder / "pic.png").write_bytes(b"\x89PNG")
    return folder


def index_line(domain: str, heading: str) -> str:
    return f"* [{heading}]({short_hash(heading)}/{domain}.md)"


def write_pick(tmp_path: Path, *lines: str) -> Path:
    pick = tmp_path / "pick.md"
    pick.write_text("".join(line + "\n" for line in lines), encoding="utf-8")
    return pick


def run(repo, pick, *extra, source=SOURCE, target=TARGET):
    return subprocess.run(
        [sys.executable, str(DOMAIN_TO_DOMAIN), str(repo), source, target, "--pick", str(pick), *extra],
        capture_output=True,
        text=True,
    )


def target_dir(repo, heading):
    return repo / TARGET / short_hash(heading)


def test_adds_picked_entries_only(repo, tmp_path):
    make_entry(repo, SOURCE, "A")
    make_entry(repo, SOURCE, "B")
    pick = write_pick(tmp_path, "# my picks", index_line(SOURCE, "A"))

    result = run(repo, pick)

    assert result.returncode == 0, result.stderr
    assert f"ADD: {TARGET}/{short_hash('A')}  A" in result.stdout
    assert "1 added, 0 updated, 0 removed, 0 unchanged, 0 failed, 0 unmatched" in result.stdout
    assert target_dir(repo, "A").is_dir()
    assert not target_dir(repo, "B").exists()
    index = (repo / TARGET / "index.md").read_text(encoding="utf-8")
    assert index == index_line(TARGET, "A") + "\n"


def test_copy_is_renamed_to_target_domain(repo, tmp_path):
    make_entry(repo, SOURCE, "A", body="alpha")
    pick = write_pick(tmp_path, index_line(SOURCE, "A"))

    run(repo, pick)

    dest = target_dir(repo, "A")
    assert sorted(p.name for p in dest.iterdir()) == ["pic.png", f"{TARGET}.md", f"{TARGET}.txt"]
    md = (dest / f"{TARGET}.md").read_text(encoding="utf-8")
    assert md.startswith("# A\n\n2026-09-11T12:00:00+00:00\n\n")
    assert f"[Plain text]({TARGET}.txt)" in md
    assert f"{SOURCE}.txt" not in md
    assert "![pic](pic.png)" in md
    assert (dest / f"{TARGET}.txt").read_text(encoding="utf-8") == "alpha\n"


def test_source_is_left_untouched(repo, tmp_path):
    src = make_entry(repo, SOURCE, "A")
    before = {p.name: p.read_bytes() for p in src.iterdir()}
    pick = write_pick(tmp_path, index_line(SOURCE, "A"))

    run(repo, pick)

    assert {p.name: p.read_bytes() for p in src.iterdir()} == before


def test_pick_by_hand_written_heading(repo, tmp_path):
    make_entry(repo, SOURCE, "Some [odd] heading?")
    pick = write_pick(tmp_path, "* [Some [odd] heading?]()")

    result = run(repo, pick)

    assert "1 added" in result.stdout
    assert target_dir(repo, "Some [odd] heading?").is_dir()


def test_rerun_is_unchanged_and_leaves_index_alone(repo, tmp_path):
    make_entry(repo, SOURCE, "A")
    pick = write_pick(tmp_path, index_line(SOURCE, "A"))
    run(repo, pick)
    index = repo / TARGET / "index.md"
    index.write_text("hand edited\n", encoding="utf-8")

    result = run(repo, pick)

    assert "0 added, 0 updated, 0 removed, 1 unchanged" in result.stdout
    assert index.read_text(encoding="utf-8") == "hand edited\n"


def test_changed_source_entry_is_updated(repo, tmp_path):
    src = make_entry(repo, SOURCE, "A", body="alpha")
    pick = write_pick(tmp_path, index_line(SOURCE, "A"))
    run(repo, pick)
    (src / "extra.png").write_bytes(b"new")
    (src / f"{SOURCE}.txt").write_text("alpha revised\n", encoding="utf-8")

    result = run(repo, pick)

    assert f"UPDATE: {TARGET}/{short_hash('A')}  A" in result.stdout
    assert "0 added, 1 updated" in result.stdout
    dest = target_dir(repo, "A")
    assert (dest / "extra.png").is_file()
    assert (dest / f"{TARGET}.txt").read_text(encoding="utf-8") == "alpha revised\n"


def test_unpicked_entry_is_removed(repo, tmp_path):
    make_entry(repo, SOURCE, "A")
    make_entry(repo, SOURCE, "B")
    run(repo, write_pick(tmp_path, index_line(SOURCE, "A"), index_line(SOURCE, "B")))

    result = run(repo, write_pick(tmp_path, index_line(SOURCE, "A")))

    assert f"REMOVE: {TARGET}/{short_hash('B')}  B" in result.stdout
    assert "0 added, 0 updated, 1 removed, 1 unchanged" in result.stdout
    assert not target_dir(repo, "B").exists()
    assert (repo / SOURCE / short_hash("B")).is_dir()  # source keeps it
    index = (repo / TARGET / "index.md").read_text(encoding="utf-8")
    assert index == index_line(TARGET, "A") + "\n"


def test_entry_gone_from_source_is_removed_and_pick_unmatched(repo, tmp_path):
    src = make_entry(repo, SOURCE, "A")
    pick = write_pick(tmp_path, index_line(SOURCE, "A"))
    run(repo, pick)
    shutil.rmtree(src)

    result = run(repo, pick)

    assert f"REMOVE: {TARGET}/{short_hash('A')}  A" in result.stdout
    assert f"UNMATCHED: {index_line(SOURCE, 'A')}" in result.stdout
    assert not target_dir(repo, "A").exists()


def test_unmatched_pick_is_reported(repo, tmp_path):
    make_entry(repo, SOURCE, "A")
    pick = write_pick(tmp_path, index_line(SOURCE, "A"), "* [Typo]()", "* [x](deadbeef/inbox.md)")

    result = run(repo, pick)

    assert "UNMATCHED: * [Typo]()" in result.stdout
    assert "UNMATCHED: * [x](deadbeef/inbox.md)" in result.stdout
    assert "1 added, 0 updated, 0 removed, 0 unchanged, 0 failed, 2 unmatched" in result.stdout


def test_dry_run_changes_nothing(repo, tmp_path):
    make_entry(repo, SOURCE, "A")
    make_entry(repo, SOURCE, "B")
    run(repo, write_pick(tmp_path, index_line(SOURCE, "B")))
    before = sorted(p.relative_to(repo) for p in (repo / TARGET).rglob("*"))

    result = run(repo, write_pick(tmp_path, index_line(SOURCE, "A")), "--dry-run")

    assert f"WOULD ADD: {TARGET}/{short_hash('A')}  A" in result.stdout
    assert f"WOULD REMOVE: {TARGET}/{short_hash('B')}  B" in result.stdout
    assert "DRY RUN — nothing changed." in result.stdout
    assert sorted(p.relative_to(repo) for p in (repo / TARGET).rglob("*")) == before


def test_dry_run_into_new_target_creates_no_folder(repo, tmp_path):
    make_entry(repo, SOURCE, "A")

    run(repo, write_pick(tmp_path, index_line(SOURCE, "A")), "--dry-run")

    assert not (repo / TARGET).exists()


def test_source_entry_whose_heading_doesnt_match_folder_fails(repo, tmp_path):
    src = make_entry(repo, SOURCE, "A")
    md = src / f"{SOURCE}.md"
    md.write_text(md.read_text(encoding="utf-8").replace("# A\n", "# A edited\n", 1), encoding="utf-8")

    result = run(repo, write_pick(tmp_path, f"* [A]({short_hash('A')}/{SOURCE}.md)"))

    assert f"FAIL: {SOURCE}/{short_hash('A')}  A edited" in result.stdout
    assert "hashes to" in result.stdout
    assert "0 added, 0 updated, 0 removed, 0 unchanged, 1 failed" in result.stdout
    assert not (repo / TARGET).exists()


def test_non_entry_folders_in_target_are_left_alone(repo, tmp_path):
    make_entry(repo, SOURCE, "A")
    keep = repo / TARGET / "notes"
    keep.mkdir(parents=True)
    (keep / "keep.md").write_text("keep\n", encoding="utf-8")

    result = run(repo, write_pick(tmp_path, index_line(SOURCE, "A")))

    assert "0 removed" in result.stdout
    assert (keep / "keep.md").is_file()


def test_entry_without_txt_is_copied(repo, tmp_path):
    make_entry(repo, SOURCE, "A", with_txt=False)

    run(repo, write_pick(tmp_path, index_line(SOURCE, "A")))

    assert sorted(p.name for p in target_dir(repo, "A").iterdir()) == ["pic.png", f"{TARGET}.md"]


@pytest.mark.parametrize(
    "args, message",
    [
        (["repo", "inbox", "--pick", "pick.md"], "required"),  # target missing
        (["repo", "inbox", "inbox", "--pick", "pick.md"], "both 'inbox'"),
        (["repo", "inbox", "a/b", "--pick", "pick.md"], "invalid domain"),
        (["repo", "inbox", "todo"], "--pick"),
        (["repo", "nosuch", "todo", "--pick", "pick.md"], "does not exist"),
    ],
)
def test_cli_argument_errors(repo, tmp_path, args, message):
    make_entry(repo, SOURCE, "A")
    write_pick(tmp_path, index_line(SOURCE, "A"))

    result = subprocess.run(
        [sys.executable, str(DOMAIN_TO_DOMAIN), *args],
        cwd=tmp_path,
        capture_output=True,
        text=True,
    )

    assert result.returncode != 0
    assert message in result.stderr
    assert not (repo / TARGET).exists()
