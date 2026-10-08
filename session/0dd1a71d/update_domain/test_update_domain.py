import hashlib
import subprocess
import sys
from pathlib import Path

import pytest

UPDATE_DOMAIN = Path(__file__).parent / "update_domain.py"

DOMAIN = "chime"


def short_hash(heading: str) -> str:
    return hashlib.md5(heading.encode("utf-8")).hexdigest()[:8]


@pytest.fixture
def repo(tmp_path):
    repo = tmp_path / "repo"
    (repo / DOMAIN).mkdir(parents=True)
    return repo


def make_entry(repo: Path, heading: str, *body_lines: str) -> Path:
    """An entry laid out like eml_to_domain writes it; returns its .md."""
    folder = repo / DOMAIN / short_hash(heading)
    folder.mkdir(parents=True)
    body = "".join(line + "\n" for line in body_lines)
    md = folder / f"{DOMAIN}.md"
    md.write_text(
        f"# {heading}\n\n*As of 2026-10-08 12:00*\n\n[Plain text]({DOMAIN}.txt)\n\n"
        f"{{% raw %}}\n{body}\n{{% endraw %}}\n",
        encoding="utf-8",
    )
    return md


def link(heading: str) -> str:
    return f"../{short_hash(heading)}/{DOMAIN}.md"


def run(repo, *extra):
    return subprocess.run(
        [sys.executable, str(UPDATE_DOMAIN), "intralink", str(repo), DOMAIN, *extra],
        capture_output=True,
        text=True,
    )


def summary(result) -> str:
    return result.stdout.strip().splitlines()[-1]


# --- what gets linked ----------------------------------------------------

def test_exact_reference_is_linked_and_text_kept(repo):
    make_entry(repo, "Todo: B?")
    a = make_entry(repo, "Todo: A", '==&gt; Also see "Todo: B?"')

    result = run(repo)

    assert result.returncode == 0, result.stderr
    assert f'==&gt; Also see "[Todo: B?]({link("Todo: B?")})"\n' in a.read_text(encoding="utf-8")
    assert "1 linked (1 exact, 0 normalized, 0 loose, 0 fuzzy)" in summary(result)
    assert "1 entries changed" in summary(result)


@pytest.mark.parametrize("cue", ["Also see", "See also", "also SEE:", "Se också", "Se även"])
def test_english_and_swedish_cues(repo, cue):
    make_entry(repo, "Todo: B")
    a = make_entry(repo, "Todo: A", f"==&gt; {cue} ”Todo: B”")

    run(repo)

    assert f"{cue} ”[Todo: B]({link('Todo: B')})”" in a.read_text(encoding="utf-8")


@pytest.mark.parametrize("arrow", ["==>", "==&gt;", "== &gt;", "=&gt;=", "> *==&gt;"])
def test_arrow_variants(repo, arrow):
    make_entry(repo, "Todo: B")
    a = make_entry(repo, "Todo: A", f'{arrow} Also see "Todo: B"')

    run(repo)

    assert f"[Todo: B]({link('Todo: B')})" in a.read_text(encoding="utf-8")


@pytest.mark.parametrize(
    "line",
    [
        'Also see "Todo: B"',  # no arrow
        '2) Also see "Todo: B"',  # no arrow
        '==&gt; Also see loos reaction to "Todo: B"',  # cue not right before the title
        '==&gt; It seems "Todo: B"',  # no cue
    ],
)
def test_only_arrow_cue_quote_lines_are_references(repo, line):
    make_entry(repo, "Todo: B")
    a = make_entry(repo, "Todo: A", line)
    before = a.read_text(encoding="utf-8")

    result = run(repo)

    assert a.read_text(encoding="utf-8") == before
    assert "0 references" in summary(result)


@pytest.mark.parametrize(
    "written",
    [
        "“Todo: B  needs  &lt;T&gt; ”",  # curly quotes, stray whitespace, HTML escapes
        '"todo: b needs <T>"',  # case, missing "?"
        '"todo: b needs <T>" (see below)',  # closing quote, then more text
    ],
)
def test_normalized_match(repo, written):
    make_entry(repo, "Todo: B needs <T>?")
    a = make_entry(repo, "Todo: A", f"==&gt; Also see {written}")

    result = run(repo)

    assert link("Todo: B needs <T>?") in a.read_text(encoding="utf-8")
    assert "1 normalized" in summary(result)


def test_link_text_is_what_was_written(repo):
    make_entry(repo, "Todo: B needs <T>?")
    a = make_entry(repo, "Todo: A", "==&gt; Also see “Todo: B  needs  &lt;T&gt; ”")

    run(repo)

    assert f"“[Todo: B  needs  &lt;T&gt;]({link('Todo: B needs <T>?')}) ”" in a.read_text(encoding="utf-8")


def test_markdown_link_and_autolink_read_as_text(repo):
    title = "Todo: Try http://x.io and y.io?"
    make_entry(repo, title)
    a = make_entry(repo, "Todo: A", '==&gt; Also see "Todo: Try <http://x.io> and [y.io](http://y.io)?"')

    run(repo)

    assert link(title) in a.read_text(encoding="utf-8")


@pytest.mark.parametrize("written", ["Consider B?", "odo: Consider B?"])
def test_loose_match_ignores_todo_prefix(repo, written):
    make_entry(repo, "Todo: Consider B?")
    a = make_entry(repo, "Todo: A", f'==&gt; Also see "{written}"')

    result = run(repo)

    assert link("Todo: Consider B?") in a.read_text(encoding="utf-8")
    assert "1 loose" in summary(result)


def test_title_with_inner_quotes_and_trailing_note(repo):
    title = 'Todo: Install Brew on Macbook Pro 17" 2009?'
    make_entry(repo, title)
    a = make_entry(repo, "Todo: A", f'==&gt; Also see "{title}" (older spelling "Cinsider")')

    run(repo)

    text = a.read_text(encoding="utf-8")
    assert f'"[{title}]({link(title)})" (older spelling "Cinsider")' in text


def test_unclosed_quote(repo):
    make_entry(repo, "Todo: B?")
    a = make_entry(repo, "Todo: A", '==&gt; Also see "Todo: B?')

    run(repo)

    assert f'"[Todo: B?]({link("Todo: B?")})' in a.read_text(encoding="utf-8")


def test_brackets_in_link_text_are_escaped(repo):
    make_entry(repo, "Todo: B [draft]")
    a = make_entry(repo, "Todo: A", '==&gt; Also see "Todo: B [draft]"')

    run(repo)

    assert f"[Todo: B \\[draft\\]]({link('Todo: B [draft]')})" in a.read_text(encoding="utf-8")


# --- fuzzy -----------------------------------------------------------------

def test_fuzzy_match_links_a_typo_and_reports_it(repo):
    title = "Todo: Consider the Peter Millard youtube video on Blum concealed hinges?"
    make_entry(repo, title)
    a = make_entry(repo, "Todo: A", f'==&gt; Also see "{title.replace("Blum", "Blüm")}"')

    result = run(repo)

    assert link(title) in a.read_text(encoding="utf-8")
    assert "FUZZY (" in result.stdout
    assert "1 fuzzy" in summary(result)


def test_fuzzy_never_crosses_different_numbers(repo):
    make_entry(repo, "Todo: Consider to document how my brew upgrade gcc to gcc13 went?")
    a = make_entry(repo, "Todo: A", '==&gt; Also see "Todo: Consider to document how my brew upgrade gcc to gcc14 went?"')
    before = a.read_text(encoding="utf-8")

    result = run(repo)

    assert a.read_text(encoding="utf-8") == before
    assert "UNRESOLVED: chime/" in result.stdout
    assert "1 unresolved" in summary(result)


def test_numbers_pick_the_right_one_of_two_near_titles(repo):
    right = "Todo: Buy a Mafell FM 1000 PV-WS (1000 W with PV control)?"
    make_entry(repo, right)
    make_entry(repo, "Todo: Buy a Mafell FM 1000 PV-WS (100W with PV control)?")
    a = make_entry(repo, "Todo: A", '==&gt; Also see "Todo: Buy a Mafell FM 1000 PV-WS (1000W with PV control)?"')

    run(repo)

    assert link(right) in a.read_text(encoding="utf-8")


def test_two_equally_near_titles_are_ambiguous(repo):
    make_entry(repo, "Todo: Consider the blue widget design for the garden shed?")
    make_entry(repo, "Todo: Consider the blue gadget design for the garden shed?")
    a = make_entry(repo, "Todo: A", '==&gt; Also see "Todo: Consider the blue wadget design for the garden shed?"')
    before = a.read_text(encoding="utf-8")

    result = run(repo)

    assert a.read_text(encoding="utf-8") == before
    assert "AMBIGUOUS: chime/" in result.stdout


# --- left alone and reported ---------------------------------------------------

def test_unresolved_is_reported_with_location(repo):
    a = make_entry(repo, "Todo: A", '==&gt; Also see "Todo: Nowhere"')

    result = run(repo)

    assert f'UNRESOLVED: chime/{a.parent.name}/chime.md:8  "Todo: Nowhere"' in result.stdout


def test_reference_to_itself_is_not_linked(repo):
    a = make_entry(repo, "Todo: A", '==&gt; Also see "Todo: A"')
    before = a.read_text(encoding="utf-8")

    result = run(repo)

    assert a.read_text(encoding="utf-8") == before
    assert "SELF: chime/" in result.stdout


def test_broken_existing_link_is_reported(repo):
    a = make_entry(repo, "Todo: A", '==&gt; Also see "[Todo: Gone](../deadbeef/chime.md)"')

    result = run(repo)

    assert "BROKEN: chime/" in result.stdout
    assert "1 broken" in summary(result)


# --- re-running, dry run, files ----------------------------------------------------

def test_rerun_changes_nothing(repo):
    make_entry(repo, 'Todo: B 17" 2009?')
    a = make_entry(
        repo, "Todo: A",
        '==&gt; Also see "Todo: B 17" 2009?" (note "x")',
        "==&gt; Se också ”Todo: b 17” 2009”",
    )
    run(repo)
    once = a.read_text(encoding="utf-8")

    result = run(repo)

    assert a.read_text(encoding="utf-8") == once
    assert "0 linked" in summary(result)
    assert "2 already linked" in summary(result)
    assert "0 entries changed" in summary(result)


def test_dry_run_changes_nothing(repo):
    make_entry(repo, "Todo: B")
    a = make_entry(repo, "Todo: A", '==&gt; Also see "Todo: B"')
    before = a.read_text(encoding="utf-8")

    result = run(repo, "--dry-run")

    assert a.read_text(encoding="utf-8") == before
    assert summary(result).startswith("DRY RUN — nothing changed.")
    assert "1 linked" in summary(result)
    assert "1 entries would change" in summary(result)


def test_crlf_line_endings_are_kept(repo):
    make_entry(repo, "Todo: B")
    a = repo / DOMAIN / "aaaa0000" / f"{DOMAIN}.md"
    a.parent.mkdir()
    a.write_bytes(b'# Todo: A\r\n\r\n==&gt; Also see "Todo: B"\r\nend\r\n')

    run(repo)

    assert a.read_bytes() == (
        f'# Todo: A\r\n\r\n==&gt; Also see "[Todo: B]({link("Todo: B")})"\r\nend\r\n'.encode()
    )


def test_missing_domain_fails(repo):
    result = subprocess.run(
        [sys.executable, str(UPDATE_DOMAIN), "intralink", str(repo), "nope"],
        capture_output=True,
        text=True,
    )

    assert result.returncode != 0
    assert "does not exist" in result.stderr
