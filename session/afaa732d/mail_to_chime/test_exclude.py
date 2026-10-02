import hashlib
from pathlib import Path

from exclude import find_exclusions, load_exclusions


def compute_hash(text: str) -> str:
    return hashlib.md5(text.encode("utf-8")).hexdigest()[:8]


def write_exclude(tmp_path: Path, content: str) -> Path:
    path = tmp_path / "exclude.md"
    path.write_text(content, encoding="utf-8")
    return path


def test_index_line_matches_by_hash_and_heading(tmp_path):
    path = write_exclude(tmp_path, "* [TODO: Wrap up TestBench](ad2d0789/chime.md)\n")

    exclusions = load_exclusions(path, compute_hash)

    [entry] = exclusions.entries
    assert entry.target_hash == "ad2d0789"
    assert entry.heading == "TODO: Wrap up TestBench"
    assert exclusions.matches("TODO: Wrap up TestBench")
    assert not exclusions.matches("TODO: Something else")
    assert len(exclusions) == 1


def test_annotated_heading_still_matches_by_hash(tmp_path):
    path = write_exclude(tmp_path, "* [TODO: Wrap up TestBench (private)](ad2d0789/chime.md)\n")

    assert load_exclusions(path, compute_hash).matches("TODO: Wrap up TestBench")


def test_hand_written_entry_matches_by_heading(tmp_path):
    path = write_exclude(tmp_path, "* [Todo: hand written]()\n")

    exclusions = load_exclusions(path, compute_hash)

    assert exclusions.entries[0].target_hash is None
    assert exclusions.matches("Todo: hand written")


def test_heading_with_brackets(tmp_path):
    path = write_exclude(tmp_path, "* [Todo: [draft] a ](b)]()\n")

    assert load_exclusions(path, compute_hash).matches("Todo: [draft] a ](b)")


def test_non_entry_lines_ignored(tmp_path):
    path = write_exclude(
        tmp_path,
        "# Private\n"
        "\n"
        "Keep these off the site:\n"
        "* [Todo: one](8c17926c/chime.md)\n"
        "  - [Todo: two](chime/5135dc9a/chime.md)\n",
    )

    exclusions = load_exclusions(path, compute_hash)

    assert [e.target_hash for e in exclusions.entries] == ["8c17926c", "5135dc9a"]
    assert [e.heading for e in exclusions.entries] == ["Todo: one", "Todo: two"]


def test_chime_hashes_are_target_and_heading_hash(tmp_path):
    path = write_exclude(tmp_path, "* [Todo: one (private)](8c17926c/chime.md)\n")
    exclusions = load_exclusions(path, compute_hash)

    assert exclusions.chime_hashes(exclusions.entries[0]) == {
        "8c17926c",
        compute_hash("Todo: one (private)"),
    }


def test_unmatched_lists_entries_that_matched_nothing(tmp_path):
    path = write_exclude(tmp_path, "* [Todo: one]()\n* [Todo: typo]()\n")
    exclusions = load_exclusions(path, compute_hash)

    exclusions.matches("Todo: one")

    assert [e.line for e in exclusions.unmatched()] == ["* [Todo: typo]()"]


def test_find_exclusions_absent_file_is_none(tmp_path):
    assert find_exclusions(tmp_path, compute_hash) is None


def test_find_exclusions_present_file(tmp_path):
    write_exclude(tmp_path, "* [Todo: one]()\n")

    exclusions = find_exclusions(tmp_path, compute_hash)

    assert exclusions is not None
    assert exclusions.source == tmp_path / "exclude.md"
