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


def run(repo, pick, mode, *extra, source=SOURCE, target=TARGET):
    return subprocess.run(
        [sys.executable, str(DOMAIN_TO_DOMAIN), mode, str(repo), source, target,
         "--pick", str(pick), *extra],
        capture_output=True,
        text=True,
    )


def target_dir(repo, heading):
    return repo / TARGET / short_hash(heading)


# --- init ---------------------------------------------------------------

def test_init_adds_picked_entries_only(repo, tmp_path):
    make_entry(repo, SOURCE, "A")
    make_entry(repo, SOURCE, "B")
    pick = write_pick(tmp_path, "# my picks", index_line(SOURCE, "A"))

    result = run(repo, pick, "init")

    assert result.returncode == 0, result.stderr
    assert f"ADD: {TARGET}/{short_hash('A')}  A" in result.stdout
    assert f"{SOURCE} -> {TARGET} (init), 1 pick entries: 1 added, 0 skipped, 0 failed, 0 unmatched" in result.stdout
    assert target_dir(repo, "A").is_dir()
    assert not target_dir(repo, "B").exists()
    index = (repo / TARGET / "index.md").read_text(encoding="utf-8")
    assert index == index_line(TARGET, "A") + "\n"


def test_init_into_existing_target_fails_and_recommends_add(repo, tmp_path):
    make_entry(repo, SOURCE, "A")
    (repo / TARGET).mkdir()

    result = run(repo, write_pick(tmp_path, index_line(SOURCE, "A")), "init")

    assert result.returncode != 0
    assert "already exists" in result.stderr
    assert "use 'add'" in result.stderr
    assert not any((repo / TARGET).iterdir())


def test_copy_is_renamed_to_target_domain(repo, tmp_path):
    make_entry(repo, SOURCE, "A", body="alpha")
    pick = write_pick(tmp_path, index_line(SOURCE, "A"))

    run(repo, pick, "init")

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

    run(repo, pick, "init")

    assert {p.name: p.read_bytes() for p in src.iterdir()} == before


def test_pick_by_hand_written_heading(repo, tmp_path):
    make_entry(repo, SOURCE, "Some [odd] heading?")
    pick = write_pick(tmp_path, "* [Some [odd] heading?]()")

    result = run(repo, pick, "init")

    assert "1 added" in result.stdout
    assert target_dir(repo, "Some [odd] heading?").is_dir()


def test_entry_without_txt_is_copied(repo, tmp_path):
    make_entry(repo, SOURCE, "A", with_txt=False)

    run(repo, write_pick(tmp_path, index_line(SOURCE, "A")), "init")

    assert sorted(p.name for p in target_dir(repo, "A").iterdir()) == ["pic.png", f"{TARGET}.md"]


def test_dry_run_init_creates_no_folder(repo, tmp_path):
    make_entry(repo, SOURCE, "A")

    result = run(repo, write_pick(tmp_path, index_line(SOURCE, "A")), "init", "--dry-run")

    assert f"WOULD ADD: {TARGET}/{short_hash('A')}  A" in result.stdout
    assert "DRY RUN — nothing changed." in result.stdout
    assert not (repo / TARGET).exists()


def test_init_where_nothing_could_be_added_leaves_no_target(repo, tmp_path):
    make_entry(repo, SOURCE, "A")

    result = run(repo, write_pick(tmp_path, "* [Typo]()"), "init")

    assert result.returncode == 1
    assert not (repo / TARGET).exists()  # so init can simply be rerun


# --- add ----------------------------------------------------------------

def test_add_into_missing_target_fails_and_recommends_init(repo, tmp_path):
    make_entry(repo, SOURCE, "A")

    result = run(repo, write_pick(tmp_path, index_line(SOURCE, "A")), "add")

    assert result.returncode != 0
    assert "does not exist" in result.stderr
    assert "use 'init'" in result.stderr
    assert not (repo / TARGET).exists()


def test_add_adds_new_entries_next_to_existing_ones(repo, tmp_path):
    make_entry(repo, SOURCE, "A")
    make_entry(repo, SOURCE, "B")
    run(repo, write_pick(tmp_path, index_line(SOURCE, "A")), "init")

    result = run(repo, write_pick(tmp_path, index_line(SOURCE, "A"), index_line(SOURCE, "B")), "add")

    assert result.returncode == 0, result.stderr
    assert f"ADD: {TARGET}/{short_hash('B')}  B" in result.stdout
    assert f"SKIP: {TARGET}/{short_hash('A')}  A (already in target)" in result.stdout
    assert f"(add), 2 pick entries: 1 added, 1 skipped, 0 failed, 0 unmatched" in result.stdout
    index = (repo / TARGET / "index.md").read_text(encoding="utf-8")
    assert index_line(TARGET, "A") in index
    assert index_line(TARGET, "B") in index


def test_add_keeps_edited_target_entry_and_notes_changed_source(repo, tmp_path):
    src = make_entry(repo, SOURCE, "A", body="alpha")
    pick = write_pick(tmp_path, index_line(SOURCE, "A"))
    run(repo, pick, "init")
    edited = target_dir(repo, "A") / f"{TARGET}.md"
    edited.write_text("# A\n\nmy edits\n", encoding="utf-8")
    (src / f"{SOURCE}.txt").write_text("alpha revised\n", encoding="utf-8")

    result = run(repo, pick, "add")

    assert result.returncode == 0, result.stderr
    assert f"SKIP: {TARGET}/{short_hash('A')}  A (already in target; source differs)" in result.stdout
    assert edited.read_text(encoding="utf-8") == "# A\n\nmy edits\n"
    assert (target_dir(repo, "A") / f"{TARGET}.txt").read_text(encoding="utf-8") == "alpha\n"


def test_add_with_nothing_new_leaves_index_alone(repo, tmp_path):
    make_entry(repo, SOURCE, "A")
    pick = write_pick(tmp_path, index_line(SOURCE, "A"))
    run(repo, pick, "init")
    index = repo / TARGET / "index.md"
    index.write_text("hand edited\n", encoding="utf-8")

    result = run(repo, pick, "add")

    assert "0 added, 1 skipped" in result.stdout
    assert index.read_text(encoding="utf-8") == "hand edited\n"


def test_add_leaves_unpicked_target_entries_alone(repo, tmp_path):
    make_entry(repo, SOURCE, "A")
    make_entry(repo, SOURCE, "B")
    run(repo, write_pick(tmp_path, index_line(SOURCE, "A"), index_line(SOURCE, "B")), "init")

    result = run(repo, write_pick(tmp_path, index_line(SOURCE, "A")), "add")

    assert result.returncode == 0, result.stderr
    assert target_dir(repo, "B").is_dir()


def test_add_keeps_entry_gone_from_source_and_reports_pick_unmatched(repo, tmp_path):
    src = make_entry(repo, SOURCE, "A")
    pick = write_pick(tmp_path, index_line(SOURCE, "A"))
    run(repo, pick, "init")
    shutil.rmtree(src)

    result = run(repo, pick, "add")

    assert result.returncode == 1
    assert f"UNMATCHED: {index_line(SOURCE, 'A')}" in result.stdout
    assert target_dir(repo, "A").is_dir()


def test_dry_run_add_changes_nothing(repo, tmp_path):
    make_entry(repo, SOURCE, "A")
    make_entry(repo, SOURCE, "B")
    run(repo, write_pick(tmp_path, index_line(SOURCE, "B")), "init")
    before = sorted(p.relative_to(repo) for p in (repo / TARGET).rglob("*"))

    result = run(repo, write_pick(tmp_path, index_line(SOURCE, "A")), "add", "--dry-run")

    assert f"WOULD ADD: {TARGET}/{short_hash('A')}  A" in result.stdout
    assert "DRY RUN — nothing changed." in result.stdout
    assert sorted(p.relative_to(repo) for p in (repo / TARGET).rglob("*")) == before


def test_non_entry_folders_in_target_are_left_alone(repo, tmp_path):
    make_entry(repo, SOURCE, "A")
    keep = repo / TARGET / "notes"
    keep.mkdir(parents=True)
    (keep / "keep.md").write_text("keep\n", encoding="utf-8")

    result = run(repo, write_pick(tmp_path, index_line(SOURCE, "A")), "add")

    assert "1 added" in result.stdout
    assert (keep / "keep.md").is_file()


# --- failures (either mode) ---------------------------------------------

def test_unmatched_pick_is_reported_and_exits_1(repo, tmp_path):
    make_entry(repo, SOURCE, "A")
    pick = write_pick(tmp_path, index_line(SOURCE, "A"), "* [Typo]()", "* [x](deadbeef/inbox.md)")

    result = run(repo, pick, "init")

    assert result.returncode == 1
    assert "UNMATCHED: * [Typo]()" in result.stdout
    assert "UNMATCHED: * [x](deadbeef/inbox.md)" in result.stdout
    assert "1 added, 0 skipped, 0 failed, 2 unmatched" in result.stdout
    assert target_dir(repo, "A").is_dir()  # the good pick is still added


def test_source_entry_whose_heading_doesnt_match_folder_fails(repo, tmp_path):
    src = make_entry(repo, SOURCE, "A")
    md = src / f"{SOURCE}.md"
    md.write_text(md.read_text(encoding="utf-8").replace("# A\n", "# A edited\n", 1), encoding="utf-8")

    result = run(repo, write_pick(tmp_path, f"* [A]({short_hash('A')}/{SOURCE}.md)"), "init")

    assert result.returncode == 1
    assert f"FAIL: {SOURCE}/{short_hash('A')}  A edited" in result.stdout
    assert "hashes to" in result.stdout
    assert "0 added, 0 skipped, 1 failed" in result.stdout
    assert not (repo / TARGET).exists()


@pytest.mark.parametrize(
    "args, message",
    [
        (["init", "repo", "inbox", "--pick", "pick.md"], "required"),  # target missing
        (["init", "repo", "inbox", "inbox", "--pick", "pick.md"], "both 'inbox'"),
        (["init", "repo", "inbox", "a/b", "--pick", "pick.md"], "invalid domain"),
        (["init", "repo", "inbox", "todo"], "--pick"),
        (["init", "repo", "nosuch", "todo", "--pick", "pick.md"], "does not exist"),
        (["repo", "inbox", "todo", "--pick", "pick.md"], "invalid choice: 'repo'"),  # mode missing
        (["sync", "repo", "inbox", "todo", "--pick", "pick.md"], "invalid choice: 'sync'"),
        (["--pick", "pick.md"], "required: mode"),
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
