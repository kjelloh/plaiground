import pytest

from eml_to_markdown import to_path


def test_to_path_missing_file_raises(tmp_path):
    fake = tmp_path / "nope.eml"
    with pytest.raises(FileNotFoundError):
        to_path(str(fake))


def test_to_path_returns_path_for_existing_file(tmp_path):
    eml = tmp_path / "sample.eml"
    eml.write_text("From: a@b.com\n")
    result = to_path(str(eml))
    assert result == eml
