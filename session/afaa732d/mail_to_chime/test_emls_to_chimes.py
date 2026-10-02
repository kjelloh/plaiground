import hashlib
from pathlib import Path

from emls_to_chimes import exclude_report
from exclude import load_exclusions


def compute_hash(text: str) -> str:
    return hashlib.md5(text.encode("utf-8")).hexdigest()[:8]


def exclusions_for(tmp_path: Path, content: str):
    path = tmp_path / "exclude.md"
    path.write_text(content, encoding="utf-8")
    return load_exclusions(path, compute_hash)


def test_report_counts_files_and_subjects_separately(tmp_path):
    exclusions = exclusions_for(tmp_path, "* [A]()\n* [B]()\n* [Typo]()\n")
    exclusions.matches("A")
    exclusions.matches("B")
    a, b = compute_hash("A"), compute_hash("B")
    excluded_files = {a: ["A.eml", "A 2.eml", "A 4.eml"], b: ["B.eml"]}
    removed = [(tmp_path / "chime" / a, "A")]

    report = "\n".join(exclude_report(exclusions, excluded_files, removed))

    assert "3 entries:" in report
    assert "4 mail files excluded, covering 2 subjects" in report
    assert "1 existing chimes removed" in report
    assert "1 excluded subjects had no existing chime" in report
    assert "whose mail is not in this folder" not in report
    assert "1 entries matched nothing" in report
    assert f"chime/{a}  A   (3 mail files excluded)" in report
    assert "  * [Typo]()" in report


def test_report_chime_removed_without_mail_in_folder(tmp_path):
    exclusions = exclusions_for(tmp_path, "* [Gone]()\n")
    gone = compute_hash("Gone")
    exclusions.entries[0].hit = True  # as remove_excluded_chimes marks it
    removed = [(tmp_path / "chime" / gone, "Gone")]

    report = "\n".join(exclude_report(exclusions, {}, removed, dry_run=True))

    assert "0 mail files would be excluded, covering 0 subjects" in report
    assert "1 existing chimes would be removed" in report
    assert "1 chimes would be removed whose mail is not in this folder" in report
    assert "Would remove 1 chime(s):" in report
    assert f"chime/{gone}  Gone   (mail not in this folder)" in report
    assert "0 entries matched nothing" in report
