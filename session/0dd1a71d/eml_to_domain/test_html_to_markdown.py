from html_to_markdown import html_to_markdown


def test_malformed_href_falls_back_to_plain_text():
    # Apple Mail's data detector can mis-parse plain text (e.g. a regex
    # snippet) into an href that urlsplit() rejects with "Invalid IPv6 URL".
    # There's no usable link target, so it should read as plain text, not
    # crash and not turn into a link of any form.
    html = '<html><body><p>See <a href="//[bad">//[bad</a> here.</p></body></html>'
    markdown, refs = html_to_markdown(html)
    assert markdown.strip() == "See //&#91;bad here."
    assert refs == []


def test_unparsable_href_falls_back_to_plain_text():
    # Same data-detector mistake, but the href doesn't match the anchor's
    # visible text (it "ate" only part of the surrounding regex snippet) —
    # there's no usable link target here at all, just mis-tagged plain text.
    html = (
        '<html><body><p>“<a href="smb://[^//">X</a>Y” '
        "matches “Hello.exe”</p></body></html>"
    )
    markdown, refs = html_to_markdown(html)
    assert "smb://" not in markdown
    assert "[X]" not in markdown and "<smb" not in markdown
    assert "XY" in markdown
    assert refs == []


def test_external_url_with_matching_label_becomes_autolink():
    html = '<html><body><a href="https://example.com/x">https://example.com/x</a></body></html>'
    markdown, refs = html_to_markdown(html)
    assert markdown.strip() == "<https://example.com/x>"


def test_local_ref_with_matching_label_becomes_bracket_link():
    html = '<html><body><a href="file.pages">file.pages</a></body></html>'
    markdown, refs = html_to_markdown(html)
    assert markdown.strip() == "[file.pages](file.pages)"
    assert refs == ["file.pages"]


def test_bracket_text_does_not_become_a_link():
    # A bare "[0-9]" in prose (e.g. a regex snippet) must not read as
    # CommonMark link syntax in the rendered output.
    html = '<html><body><p>&quot;\\d&quot; is short for [0-9]</p></body></html>'
    markdown, refs = html_to_markdown(html)
    assert "&#91;0-9&#93;" in markdown
    assert "[0-9]" not in markdown


def test_angle_bracket_text_is_entity_escaped():
    # Raw "<...>" in markdown source is valid inline HTML per CommonMark, so
    # a renderer's HTML parser (e.g. Jekyll's Kramdown) will try to parse it
    # as a tag. A mail with many literal "<...>" references (e.g. "<Settings>
    # menu") must not leave raw angle brackets in the output — on a large
    # real mail (~4300 such sequences) this made Kramdown's HTML tag parser
    # recurse once per occurrence and blow the Ruby call stack.
    html = "<html><body><p>Set &lt;Connection Type&gt; to &lt;TCP/IP&gt;.</p></body></html>"
    markdown, refs = html_to_markdown(html)
    assert "<" not in markdown and ">" not in markdown
    assert "&lt;Connection Type&gt; to &lt;TCP/IP&gt;." in markdown


def test_bracket_text_does_not_collide_with_latex_math_delimiters():
    # "\[" / "\]" is valid CommonMark escaping for a literal bracket, but
    # it's also the LaTeX display-math delimiter many renderers (e.g. VS
    # Code's Markdown preview) recognize — so backslash-escaping a bracket
    # right before an underscore can produce a broken math block instead of
    # plain text. Entity-escaping sidesteps this: no renderer treats "&#91;"
    # as the start of a math block.
    html = "<html><body><p>matches [A-Za-z0-9_]</p></body></html>"
    markdown, refs = html_to_markdown(html)
    assert "\\[" not in markdown and "\\]" not in markdown
    assert "&#91;A-Za-z0-9_&#93;" in markdown


# -- line breaks ---------------------------------------------------------
# A lone newline in markdown is a *soft* break: renderers join the two
# lines. Every line break the mail shows must therefore come out as a hard
# break ("  " before the newline) or a paragraph break (blank line).


def test_br_becomes_hard_break():
    # The trailing-whitespace cleanup used to strip the "  " a <br> emits,
    # silently turning every <br> into a soft break.
    markdown, _ = html_to_markdown("<div>first<br>second</div>")
    assert markdown == "first  \nsecond\n"


def test_plain_text_mail_lines_stay_apart():
    # eml_to_html wraps a plain-text-only mail's lines this way.
    markdown, _ = html_to_markdown("<p>* Check in<br>\n* Wrap-up</p>")
    assert markdown == "\\* Check in  \n\\* Wrap-up\n"


def test_hard_break_dropped_at_paragraph_end():
    markdown, _ = html_to_markdown("<div>a<br></div><div>b<br></div>")
    assert markdown == "a\n\nb\n"


def test_div_lines_in_blockquote_become_hard_breaks():
    markdown, _ = html_to_markdown(
        "<blockquote><div>Standards documents</div><div>J1962 – connector</div></blockquote>"
    )
    assert markdown == "> Standards documents  \n> J1962 – connector\n"


def test_div_lines_in_list_item_become_hard_breaks():
    markdown, _ = html_to_markdown("<ul><li><div>a</div><div>b</div></li><li>c</li></ul>")
    assert markdown == "- a  \n  b\n- c\n"


def test_pre_wrap_newlines_become_hard_breaks():
    # Text pasted from e.g. YouTube keeps real newlines under
    # white-space: pre-wrap; a browser shows them as line breaks.
    markdown, _ = html_to_markdown(
        '<div><span style="white-space: pre-wrap">Primer:  \nHaven:\nWorkbench</span></div>'
    )
    assert markdown == "Primer:  \nHaven:  \nWorkbench\n"


def test_apple_tab_span_is_not_a_line_break():
    markdown, _ = html_to_markdown(
        '<div><span class="Apple-tab-span" style="white-space:pre">\t</span>==&gt; note</div>'
    )
    assert markdown == "==&gt; note\n"


def test_empty_emphasis_leaves_no_stray_marker_lines():
    markdown, _ = html_to_markdown(
        "<blockquote><div><i>About</i></div><div><i><br></i></div><div><i>SBN</i></div></blockquote>"
    )
    assert "*\n" not in markdown.replace("*About*", "").replace("*SBN*", "")
    assert "*About*" in markdown and "*SBN*" in markdown


def test_emphasis_marker_moves_past_leading_space():
    # "** x**" isn't emphasis in CommonMark; " **x**" is.
    markdown, _ = html_to_markdown("<div>Note:<b> WE ARE HERE</b></div>")
    assert markdown == "Note: **WE ARE HERE**\n"
