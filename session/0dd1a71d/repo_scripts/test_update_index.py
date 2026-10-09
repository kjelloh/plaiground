import subprocess
import sys
from pathlib import Path

from init_new import ensure_entry_folder

UPDATE_INDEX = Path(__file__).parent / "update_index.py"


def run_update_index(base_dir: Path, namespace: str) -> list[str]:
    subprocess.run(
        [sys.executable, str(UPDATE_INDEX), namespace], cwd=base_dir, check=True, capture_output=True
    )
    return (base_dir / namespace / "index.md").read_text(encoding="utf-8").splitlines()


def headings(lines: list[str]) -> list[str]:
    return [line[len("* ["):line.index("](")] for line in lines]


def test_index_is_sorted_on_heading_not_hash(tmp_path):
    for heading in ("Todo: zebra", "Todo: apa", "Todo: Banan", "Todo: åka", "Todo: Äpple"):
        ensure_entry_folder("note", heading, tmp_path)

    lines = run_update_index(tmp_path, "note")

    # as-is code point order: uppercase before lowercase, Ä (U+00C4) before å (U+00E5)
    assert headings(lines) == ["Todo: Banan", "Todo: apa", "Todo: zebra", "Todo: Äpple", "Todo: åka"]


def test_index_line_links_to_entry(tmp_path):
    md, _ = ensure_entry_folder("note", "Todo: apa", tmp_path)

    lines = run_update_index(tmp_path, "note")

    assert lines == [f"* [Todo: apa]({md.parent.name}/note.md)"]


def test_same_heading_is_ordered_on_hash(tmp_path):
    for name in ("ffff0000", "0000ffff"):
        folder = tmp_path / "note" / name
        folder.mkdir(parents=True)
        (folder / "note.md").write_text("# Same\n", encoding="utf-8")

    lines = run_update_index(tmp_path, "note")

    assert lines == ["* [Same](0000ffff/note.md)", "* [Same](ffff0000/note.md)"]


def test_folders_without_entry_are_skipped(tmp_path):
    ensure_entry_folder("note", "Todo: apa", tmp_path)
    (tmp_path / "note" / "not_an_entry").mkdir()

    assert headings(run_update_index(tmp_path, "note")) == ["Todo: apa"]
