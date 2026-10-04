import hashlib

import pytest

from init_new import (
    HashCollisionError,
    compute_hash,
    ensure_entry_folder,
    read_hash_tag,
)


def md5(text: str) -> str:
    return hashlib.md5(text.encode("utf-8")).hexdigest()


def test_new_entry_has_heading_then_full_hash_tag(tmp_path):
    file_path, created = ensure_entry_folder("note", "Hello world", base_dir=tmp_path)

    assert created
    assert file_path == tmp_path / "note" / md5("Hello world")[:8] / "note.md"
    assert file_path.read_text(encoding="utf-8") == f"# Hello world\n\n#{md5('Hello world')}\n\n"


def test_folder_name_is_prefix_of_hash_tag(tmp_path):
    file_path, _ = ensure_entry_folder("note", "Åäö: subject?", base_dir=tmp_path)

    full_hash = read_hash_tag(file_path)
    assert full_hash == md5("Åäö: subject?")
    assert full_hash.startswith(file_path.parent.name)
    assert compute_hash("Åäö: subject?") == file_path.parent.name


def test_existing_entry_is_left_untouched(tmp_path):
    file_path, _ = ensure_entry_folder("note", "Hello", base_dir=tmp_path)
    file_path.write_text(file_path.read_text(encoding="utf-8") + "body\n", encoding="utf-8")

    again, created = ensure_entry_folder("note", "Hello", base_dir=tmp_path)

    assert again == file_path
    assert not created
    assert file_path.read_text(encoding="utf-8").endswith("body\n")


def test_existing_entry_without_tag_is_accepted(tmp_path):
    folder = tmp_path / "note" / compute_hash("Old")
    folder.mkdir(parents=True)
    (folder / "note.md").write_text("# Old\n\n", encoding="utf-8")

    _, created = ensure_entry_folder("note", "Old", base_dir=tmp_path)

    assert not created


def test_short_hash_collision_is_detected(tmp_path):
    folder = tmp_path / "note" / compute_hash("Mine")
    folder.mkdir(parents=True)
    other = md5("Someone else")
    (folder / "note.md").write_text(f"# Someone else\n\n#{other}\n\n", encoding="utf-8")

    with pytest.raises(HashCollisionError):
        ensure_entry_folder("note", "Mine", base_dir=tmp_path)


def test_read_hash_tag_none_without_tag(tmp_path):
    md = tmp_path / "x.md"
    md.write_text("# Heading\n\n#not-a-hash\n", encoding="utf-8")

    assert read_hash_tag(md) is None
