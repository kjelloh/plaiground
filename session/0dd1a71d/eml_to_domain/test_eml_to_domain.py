import hashlib
import shutil
import subprocess
import sys
from datetime import datetime
from pathlib import Path

import pytest

import eml_to_html
from eml_to_domain import EntrySupersededError, eml_file_to_entry, read_entry_date

SITE_REPO_INIT_NEW = Path(__file__).parent.parent / "site_repo" / "init_new.py"


@pytest.fixture(autouse=True)
def init_new_in_tmp(tmp_path):
    """eml_file_to_entry loads init_new.py from base_dir (the target
    repo) — give tmp_path a copy of the real one, like any repo this
    mechanism is pointed at would already have."""
    shutil.copy2(SITE_REPO_INIT_NEW, tmp_path / "init_new.py")


@pytest.fixture(params=["mail", "note"])
def domain(request):
    """Every test runs against two domains: nothing may assume a fixed one."""
    return request.param

SIMPLE_HTML_EML = (
    "Subject: Hello\r\n"
    "From: foo@bar.se\r\n"
    "Date: Fri, 11 Sep 2026 12:00:00 +0000\r\n"
    'Content-Type: text/html; charset="utf-8"\r\n'
    "\r\n"
    "<html><body><p>Body <strong>text</strong></p></body></html>\r\n"
)

SIMPLE_PLAIN_EML = (
    "Subject: Plain\r\n"
    "From: foo@bar.se\r\n"
    "Date: Fri, 11 Sep 2026 12:00:00 +0000\r\n"
    "Content-Type: text/plain; charset=utf-8\r\n"
    "\r\n"
    "First paragraph.\r\n"
    "\r\n"
    "Second paragraph,\r\n"
    "with a line break.\r\n"
)


def write_eml(tmp_path: Path, name: str, content: str) -> Path:
    eml_path = tmp_path / name
    eml_path.write_text(content, encoding="utf-8")
    return eml_path


def test_eml_file_to_entry_creates_entry(tmp_path, domain):
    eml_path = write_eml(tmp_path, "test.eml", SIMPLE_HTML_EML)

    entry_path = eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)

    assert entry_path == tmp_path / domain / entry_path.parent.name / f"{domain}.md"
    content = entry_path.read_text(encoding="utf-8")
    assert content.startswith("# Hello\n\n")  # from the mail's Subject, not the filename
    assert content.startswith("# Hello\n\n*As of 2026-09-11 12:00*\n\n")
    assert entry_path.parent.name == hashlib.md5(b"Hello").hexdigest()[:8]
    assert "Body **text**" in content


def test_eml_file_to_entry_cleans_up_scratch_dir(tmp_path, domain):
    eml_path = write_eml(tmp_path, "test.eml", SIMPLE_HTML_EML)

    entry_path = eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)

    assert not (entry_path.parent / "_html_scratch").exists()
    assert not (entry_path.parent / "test.md").exists()


def test_eml_file_to_entry_reprocessing_same_mail_is_superseded(tmp_path, domain):
    eml_path = write_eml(tmp_path, "test.eml", SIMPLE_HTML_EML)

    eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)

    with pytest.raises(EntrySupersededError):
        eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)


def test_eml_file_to_entry_keys_by_subject_not_filename(tmp_path, domain):
    # Apple Mail's exporter appends " 2", " 3", ... to the *filename* when
    # several exported mails share a subject — the real Subject: header is
    # identical across them, and that's what must key the entry.
    older = write_eml(tmp_path, "todo.eml", SIMPLE_HTML_EML)
    newer_raw = SIMPLE_HTML_EML.replace(
        "Date: Fri, 11 Sep 2026 12:00:00 +0000",
        "Date: Sat, 12 Sep 2026 09:00:00 +0000",
    ).replace("Body <strong>text</strong>", "Updated <strong>text</strong>")
    newer = write_eml(tmp_path, "todo 2.eml", newer_raw)

    older_entry = eml_file_to_entry(older, base_dir=tmp_path, domain=domain)
    newer_entry = eml_file_to_entry(newer, base_dir=tmp_path, domain=domain)

    assert older_entry == newer_entry  # same subject -> same entry folder
    content = newer_entry.read_text(encoding="utf-8")
    assert "Updated **text**" in content
    assert "Body **text**" not in content  # old revision's content is gone


def test_eml_file_to_entry_older_revision_is_superseded_and_leaves_content_untouched(
    tmp_path, domain
):
    newer = write_eml(
        tmp_path,
        "todo.eml",
        SIMPLE_HTML_EML.replace(
            "Date: Fri, 11 Sep 2026 12:00:00 +0000",
            "Date: Sat, 12 Sep 2026 09:00:00 +0000",
        ),
    )
    older_raw = SIMPLE_HTML_EML.replace(
        "Body <strong>text</strong>", "Stale <strong>text</strong>"
    )
    older = write_eml(tmp_path, "todo 2.eml", older_raw)

    entry_path = eml_file_to_entry(newer, base_dir=tmp_path, domain=domain)

    with pytest.raises(EntrySupersededError):
        eml_file_to_entry(older, base_dir=tmp_path, domain=domain)

    content = entry_path.read_text(encoding="utf-8")
    assert "Body **text**" in content
    assert "Stale **text**" not in content


def test_eml_file_to_entry_copies_inline_image(tmp_path, domain):
    raw = (
        "Subject: Pic\r\n"
        "From: foo@bar.se\r\n"
        'Content-Type: multipart/related; boundary="B"\r\n'
        "\r\n"
        "--B\r\n"
        'Content-Type: text/html; charset="utf-8"\r\n'
        "\r\n"
        '<html><body><img src="cid:img1" alt="a pic"></body></html>\r\n'
        "--B\r\n"
        "Content-Type: image/png\r\n"
        "Content-ID: <img1>\r\n"
        "Content-Disposition: inline; filename=pic.png\r\n"
        "Content-Transfer-Encoding: base64\r\n"
        "\r\n"
        "iVBORw0KGgo=\r\n"
        "--B--\r\n"
    )
    eml_path = write_eml(tmp_path, "pic.eml", raw)

    entry_path = eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)

    assert (entry_path.parent / "pic.png").is_file()
    assert "![a pic](pic.png)" in entry_path.read_text(encoding="utf-8")


def test_eml_file_to_entry_plain_text_fallback(tmp_path, domain):
    eml_path = write_eml(tmp_path, "plain.eml", SIMPLE_PLAIN_EML)

    entry_path = eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)

    content = entry_path.read_text(encoding="utf-8")
    assert "First paragraph." in content
    assert "Second paragraph," in content
    assert "with a line break." in content


def test_eml_file_to_entry_images_only_fallback(tmp_path, domain):
    raw = (
        "Subject: Photo\r\n"
        "From: foo@bar.se\r\n"
        'Content-Type: multipart/mixed; boundary="B"\r\n'
        "\r\n"
        "--B\r\n"
        "Content-Type: image/png\r\n"
        "Content-Disposition: inline; filename=photo.png\r\n"
        "Content-Transfer-Encoding: base64\r\n"
        "\r\n"
        "iVBORw0KGgo=\r\n"
        "--B--\r\n"
    )
    eml_path = write_eml(tmp_path, "photo.eml", raw)

    entry_path = eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)

    assert (entry_path.parent / "photo.png").is_file()
    assert "![](photo.png)" in entry_path.read_text(encoding="utf-8")


def test_eml_file_to_entry_document_only_fallback(tmp_path, domain):
    raw = (
        "Subject: Document only\r\n"
        "From: foo@bar.se\r\n"
        'Content-Type: multipart/mixed; boundary="B"\r\n'
        "\r\n"
        "--B\r\n"
        "Content-Type: application/x-iwork-pages-sffpages\r\n"
        "Content-Disposition: attachment; filename=Plans.pages\r\n"
        "Content-Transfer-Encoding: base64\r\n"
        "\r\n"
        "AAAA\r\n"
        "--B--\r\n"
    )
    eml_path = write_eml(tmp_path, "doc.eml", raw)

    entry_path = eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)

    content = entry_path.read_text(encoding="utf-8")
    assert (entry_path.parent / "Plans.pages").is_file()
    assert "## Attachments" in content
    assert "[Plans.pages](Plans.pages)" in content


def test_eml_file_to_entry_document_attachment_alongside_text(tmp_path, domain):
    raw = (
        "Subject: With attachment\r\n"
        "From: foo@bar.se\r\n"
        'Content-Type: multipart/mixed; boundary="B"\r\n'
        "\r\n"
        "--B\r\n"
        'Content-Type: text/html; charset="utf-8"\r\n'
        "\r\n"
        "<html><body><p>See attached.</p></body></html>\r\n"
        "--B\r\n"
        "Content-Type: application/x-iwork-pages-sffpages\r\n"
        "Content-Disposition: attachment; filename=Plans.pages\r\n"
        "Content-Transfer-Encoding: base64\r\n"
        "\r\n"
        "AAAA\r\n"
        "--B--\r\n"
    )
    eml_path = write_eml(tmp_path, "withdoc.eml", raw)

    entry_path = eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)

    content = entry_path.read_text(encoding="utf-8")
    assert (entry_path.parent / "Plans.pages").is_file()
    assert "See attached." in content
    assert "## Attachments" in content
    assert "[Plans.pages](Plans.pages)" in content


def test_eml_file_to_entry_wraps_body_in_liquid_raw(tmp_path, domain):
    """Mail content with Liquid-looking "{{" / "{%" (e.g. C++ brace-init)
    must not break the Jekyll build — see session.md 20261001 for the real
    4433-mail run that hit this: some shapes abort the entire site build,
    not just render oddly."""
    raw = (
        "Subject: Braces\r\n"
        "From: foo@bar.se\r\n"
        'Content-Type: text/plain; charset="utf-8"\r\n'
        "\r\n"
        'std::array<std::array<int,3>,3> a{{{{-1,-1,-1}},{{-1,-1,-1}}}};\r\n'
    )
    eml_path = write_eml(tmp_path, "braces.eml", raw)

    entry_path = eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)

    content = entry_path.read_text(encoding="utf-8")
    assert "{% raw %}" in content
    assert "{% endraw %}" in content
    assert "a{{{{-1,-1,-1}},{{-1,-1,-1}}}};" in content
    raw_start = content.index("{% raw %}")
    endraw_start = content.index("{% endraw %}")
    body_start = content.index("a{{{{-1,-1,-1}}")
    assert raw_start < body_start < endraw_start


def test_eml_file_to_entry_writes_and_links_plain_text(tmp_path, domain):
    eml_path = write_eml(tmp_path, "plain.eml", SIMPLE_PLAIN_EML)

    entry_path = eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)

    txt_path = entry_path.parent / f"{domain}.txt"
    assert txt_path.read_bytes().startswith(b"\xef\xbb\xbf")  # UTF-8 BOM, for browsers
    assert txt_path.read_text(encoding="utf-8-sig") == (
        "First paragraph.\n\nSecond paragraph,\nwith a line break.\n"
    )
    content = entry_path.read_text(encoding="utf-8")
    assert content.startswith(
        "# Plain\n\n*As of 2026-09-11 12:00*\n\n"
        f"[Plain text]({domain}.txt)\n\n"
        "{% raw %}\n"
    )


def test_eml_file_to_entry_no_plain_text_part_writes_no_txt(tmp_path, domain):
    eml_path = write_eml(tmp_path, "test.eml", SIMPLE_HTML_EML)

    entry_path = eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)

    assert not (entry_path.parent / f"{domain}.txt").exists()
    assert f"{domain}.txt" not in entry_path.read_text(encoding="utf-8")


def test_eml_file_to_entry_ignores_attached_txt_file(tmp_path, domain):
    raw = (
        "Subject: Attached txt\r\n"
        "From: foo@bar.se\r\n"
        'Content-Type: multipart/mixed; boundary="B"\r\n'
        "\r\n"
        "--B\r\n"
        'Content-Type: text/html; charset="utf-8"\r\n'
        "\r\n"
        "<html><body><p>See attached.</p></body></html>\r\n"
        "--B\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Content-Disposition: attachment; filename=notes.txt\r\n"
        "\r\n"
        "Not the body.\r\n"
        "--B--\r\n"
    )
    eml_path = write_eml(tmp_path, "att.eml", raw)

    entry_path = eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)

    assert not (entry_path.parent / f"{domain}.txt").exists()


def test_eml_file_to_entry_newer_revision_replaces_plain_text(tmp_path, domain):
    older = write_eml(tmp_path, "plain.eml", SIMPLE_PLAIN_EML)
    newer_raw = SIMPLE_PLAIN_EML.replace(
        "Date: Fri, 11 Sep 2026 12:00:00 +0000",
        "Date: Sat, 12 Sep 2026 09:00:00 +0000",
    ).replace("First paragraph.", "Updated paragraph.")
    newer = write_eml(tmp_path, "plain 2.eml", newer_raw)

    eml_file_to_entry(older, base_dir=tmp_path, domain=domain)
    entry_path = eml_file_to_entry(newer, base_dir=tmp_path, domain=domain)

    text = (entry_path.parent / f"{domain}.txt").read_text(encoding="utf-8-sig")
    assert "Updated paragraph." in text
    assert "First paragraph." not in text


def test_eml_file_to_entry_no_content_raises(tmp_path, domain):
    raw = (
        "Subject: Nothing\r\n"
        "From: foo@bar.se\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Content-Transfer-Encoding: base64\r\n"
        "\r\n"
        "AAAA\r\n"
    )
    eml_path = write_eml(tmp_path, "empty.eml", raw)

    with pytest.raises(eml_to_html.NoRenderablePartError):
        eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)


# The sample emls stay in the session they were added in (read-only here).
SAMPLE_EML_DIR = Path(__file__).parent.parent.parent / "afaa732d" / "example_eml"


def test_parse_sample_eml():
    eml_dir = SAMPLE_EML_DIR
    eml_paths = list(eml_dir.glob("*.eml"))
    assert eml_paths, f"expected at least one sample .eml in {eml_dir}"


SITE_REPO_UPDATE_INDEX = SITE_REPO_INIT_NEW.parent / "update_index.py"
EML_TO_DOMAIN = Path(__file__).parent / "eml_to_domain.py"


def run_cli(*args, cwd):
    return subprocess.run(
        [sys.executable, str(EML_TO_DOMAIN), *map(str, args)],
        cwd=cwd,
        capture_output=True,
        text=True,
    )


def test_same_mail_into_two_domains_gives_same_hash_folder(tmp_path):
    eml_path = write_eml(tmp_path, "test.eml", SIMPLE_PLAIN_EML)

    mail = eml_file_to_entry(eml_path, base_dir=tmp_path, domain="mail")
    note = eml_file_to_entry(eml_path, base_dir=tmp_path, domain="note")

    assert mail == tmp_path / "mail" / mail.parent.name / "mail.md"
    assert note == tmp_path / "note" / mail.parent.name / "note.md"
    assert (mail.parent / "mail.txt").is_file()
    assert (note.parent / "note.txt").is_file()


def test_invalid_domain_writes_nothing(tmp_path):
    eml_path = write_eml(tmp_path, "test.eml", SIMPLE_HTML_EML)

    with pytest.raises(ValueError):
        eml_file_to_entry(eml_path, base_dir=tmp_path, domain="not a domain")

    assert not [p for p in tmp_path.iterdir() if p.is_dir() and p.name != "__pycache__"]


def test_read_entry_date_reads_as_of_line(tmp_path):
    md = tmp_path / "x.md"
    md.write_text(
        "# X\n\n*As of 2026-09-11 12:00*\n\n{% raw %}\nbody\n{% endraw %}\n",
        encoding="utf-8",
    )

    assert read_entry_date(md) == datetime(2026, 9, 11, 12, 0)


def test_read_entry_date_none_without_as_of_line(tmp_path):
    # e.g. an entry in the earlier layout, with a "mail-date" comment: it
    # reads as undated, so the next dated mail rewrites it in this layout.
    md = tmp_path / "x.md"
    md.write_text("# X\n\n<!-- mail-date: 2026-09-11T12:00:00+00:00 -->\n\nbody\n", encoding="utf-8")

    assert read_entry_date(md) is None


def test_read_entry_date_none_for_invalid_date(tmp_path):
    md = tmp_path / "x.md"
    md.write_text("# X\n\n*As of 2026-13-45 12:00*\n", encoding="utf-8")

    assert read_entry_date(md) is None


def test_read_entry_date_ignores_as_of_line_inside_mail_body(tmp_path):
    md = tmp_path / "x.md"
    md.write_text(
        "# X\n\n{% raw %}\n*As of 2026-09-11 12:00*\n{% endraw %}\n",
        encoding="utf-8",
    )

    assert read_entry_date(md) is None


def test_as_of_is_the_mails_own_wall_clock_time_to_the_minute(tmp_path, domain):
    eml_path = write_eml(
        tmp_path,
        "test.eml",
        SIMPLE_HTML_EML.replace("Fri, 11 Sep 2026 12:00:00 +0000", "Fri, 11 Sep 2026 09:44:05 +0100"),
    )

    content = eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain).read_text(encoding="utf-8")

    assert [line for line in content.splitlines() if "2026-09-11" in line] == ["*As of 2026-09-11 09:44*"]


def test_mail_in_same_minute_is_superseded(tmp_path, domain):
    first = write_eml(tmp_path, "todo.eml", SIMPLE_HTML_EML)
    same_minute = write_eml(
        tmp_path, "todo 2.eml", SIMPLE_HTML_EML.replace("12:00:00 +0000", "12:00:30 +0000")
    )

    eml_file_to_entry(first, base_dir=tmp_path, domain=domain)
    with pytest.raises(EntrySupersededError):
        eml_file_to_entry(same_minute, base_dir=tmp_path, domain=domain)


def test_mail_without_date_header_gets_no_as_of_and_is_replaced_by_dated_one(tmp_path, domain):
    undated = write_eml(
        tmp_path, "todo.eml", SIMPLE_HTML_EML.replace("Date: Fri, 11 Sep 2026 12:00:00 +0000\r\n", "")
    )
    dated = write_eml(
        tmp_path, "todo 2.eml", SIMPLE_HTML_EML.replace("Body <strong>text</strong>", "Dated <strong>text</strong>")
    )

    entry_path = eml_file_to_entry(undated, base_dir=tmp_path, domain=domain)
    assert "As of" not in entry_path.read_text(encoding="utf-8")

    eml_file_to_entry(dated, base_dir=tmp_path, domain=domain)
    assert "Dated **text**" in entry_path.read_text(encoding="utf-8")
    with pytest.raises(EntrySupersededError):
        eml_file_to_entry(undated, base_dir=tmp_path, domain=domain)


def test_unparsable_date_header_gets_no_as_of(tmp_path, domain):
    eml_path = write_eml(
        tmp_path,
        "odd.eml",
        SIMPLE_HTML_EML.replace("Date: Fri, 11 Sep 2026 12:00:00 +0000", "Date: sometime  last week"),
    )

    entry_path = eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)

    assert "As of" not in entry_path.read_text(encoding="utf-8")
    assert read_entry_date(entry_path) is None


def test_entry_in_earlier_layout_is_rewritten_by_same_mail(tmp_path, domain):
    eml_path = write_eml(tmp_path, "test.eml", SIMPLE_HTML_EML)
    entry_path = eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)
    # Turn it into the earlier layout: a "mail-date" comment instead.
    old = entry_path.read_text(encoding="utf-8").replace(
        "*As of 2026-09-11 12:00*", "<!-- mail-date: 2026-09-11T12:00:00+00:00 -->"
    )
    entry_path.write_text(old, encoding="utf-8")

    eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)  # not superseded

    content = entry_path.read_text(encoding="utf-8")
    assert "*As of 2026-09-11 12:00*" in content
    assert "mail-date" not in content
    with pytest.raises(EntrySupersededError):
        eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)


def test_hand_edited_as_of_protects_entry_from_older_mail(tmp_path, domain):
    eml_path = write_eml(tmp_path, "test.eml", SIMPLE_HTML_EML)
    entry_path = eml_file_to_entry(eml_path, base_dir=tmp_path, domain=domain)
    edited = entry_path.read_text(encoding="utf-8").replace(
        "*As of 2026-09-11 12:00*", "*As of 2026-10-08 14:30*"
    ).replace("Body **text**", "Edited by hand")
    entry_path.write_text(edited, encoding="utf-8")
    newer_mail = write_eml(
        tmp_path, "test 2.eml", SIMPLE_HTML_EML.replace("Fri, 11 Sep 2026", "Wed, 30 Sep 2026")
    )

    with pytest.raises(EntrySupersededError):
        eml_file_to_entry(newer_mail, base_dir=tmp_path, domain=domain)
    assert "Edited by hand" in entry_path.read_text(encoding="utf-8")


def test_cli_requires_domain(tmp_path):
    eml_path = write_eml(tmp_path, "test.eml", SIMPLE_HTML_EML)

    result = run_cli(eml_path, "-o", tmp_path, cwd=tmp_path)

    assert result.returncode != 0
    assert "--domain" in result.stderr
    assert not [p for p in tmp_path.iterdir() if p.is_dir()]


def test_cli_rejects_invalid_domain(tmp_path):
    eml_path = write_eml(tmp_path, "test.eml", SIMPLE_HTML_EML)

    result = run_cli(eml_path, "--domain", "a/b", "-o", tmp_path, cwd=tmp_path)

    assert result.returncode != 0
    assert "invalid domain" in result.stderr


def test_cli_creates_entry_and_domain_index(tmp_path, domain):
    shutil.copy2(SITE_REPO_UPDATE_INDEX, tmp_path / "update_index.py")
    eml_path = write_eml(tmp_path, "test.eml", SIMPLE_HTML_EML)

    result = run_cli(eml_path, "--domain", domain, "-o", tmp_path, cwd=tmp_path)

    assert result.returncode == 0, result.stderr
    short_hash = hashlib.md5(b"Hello").hexdigest()[:8]
    assert (tmp_path / domain / short_hash / f"{domain}.md").is_file()
    index = (tmp_path / domain / "index.md").read_text(encoding="utf-8")
    assert index == f"* [Hello]({short_hash}/{domain}.md)\n"


def test_cli_second_run_is_superseded(tmp_path, domain):
    eml_path = write_eml(tmp_path, "test.eml", SIMPLE_HTML_EML)
    assert run_cli(eml_path, "--domain", domain, "-o", tmp_path, cwd=tmp_path).returncode == 0

    result = run_cli(eml_path, "--domain", domain, "-o", tmp_path, cwd=tmp_path)

    assert result.returncode != 0
    assert "superseded" in result.stderr
