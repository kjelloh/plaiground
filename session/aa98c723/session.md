# Consider take 2 on eml-to-chime python mechanism?

## 20260930

I now ran a test on my Todo-mails.

```sh
% ./emls_to_chimes.py ~/Downloads/mail_export/eml > output.log 2>&1
```

As it turns out it reports 4414 OK and 16 FAIL.

* [output.log](./output.log)
* [chimes](./chime/index.md)

So it seems we are not yet able to process mails woth no text but with image(s)?

```sh
FAIL: Todo_ Åk så här från Kallhäll till "Pernillas fik" på Spångavägen.eml
    └── NoRenderablePartError: No text/html or text/plain part found in the message.
```

So how can we refactor the eml-to-chime scripts to create a chime.md with the images also when the original mail has no text parts?

I asked Claude to vibe code me a solution and it did.

Now I still get 10 FAIL on my Todo-mails. I read the log to see what failed.

```sh
FAIL: Todo_ Build House - Consider to use google maps to estimate travel distance by car to Stockholm of potential locations?.eml
    └── NoRenderablePartError: No text/html, text/plain or image part found in the message.
...
```

AHA! So this mail contains an Apple Pages document.

Is there a way we can transform this into something 'web renderable'?

* Or, perhaps just provide and link the document in the produced markdown?

I asked Claude to vide code a solution for me.

```sh
Done. Summary of the refactor:

eml_to_html.py:
- New document_parts() — generalizes beyond Apple Pages to any non-image, non-body attachment (PDF, docx, zip, Pages, etc.), matched by Content-Disposition or a filename.
- New write_part() — factored out the shared "decode payload → unique local filename → write" logic that image_parts() and document_parts() both need now.
- New attachments_html() / append_attachments() — builds an <h2>Attachments</h2> + <ul> of <a href="local-name">display-name</a> links, appended before </body> regardless of which branch produced the primary content (real HTML, plain-text fallback, images-only fallback, or — new — a documents-only fallback for mails with nothing but an attachment).
- NoRenderablePartError now only fires when a mail has literally nothing extractable: no text, no HTML, no images, no documents.

html_to_markdown.py: fixed a latent bug this surfaced — when link text equals the href, it emitted a CommonMark autolink (<name>), which is only valid for absolute URIs with a scheme. For local relative paths (like our attachment links, where filename = both text and href) that's invalid markdown. Now local refs always get proper [text](target) form.

Tests: added coverage for a documents-only mail, and text+document mixed, replacing the now-outdated "attachment still raises" test with one that verifies attachments correctly participate in the fallback chain.

Verified: all 10 tests pass; batch run over all 5 .eml files in this session (including your new Pages-attachment sample) — 5 ok, 0 failed, 5 total; the Pages attachment renders as [En timme från Stockholm med bil.pages](<En timme från Stockholm med bil.pages>), a correct download link.

✻ Baked for 3m 42s · done 1:39 PM
```

Now I am down to 3 FAIL. I looked into the log.

```sh
markdown -> /Users/kjell-olovhogdahl/Documents/GitHub/plaiground/session/aa98c723/chime/d2a69299/Todo_ Programming - Consider to check out the site Techie Delight with what seems cool C++ solutions to a long list of problems.mdFAIL: Todo_ Programming - Consider to learn and document about regular expression coding?.eml
    └── ValueError: Invalid IPv6 URL
```

There seems to be two issues to attend to?

* The 'FAIL:...' output does not start on a new line?
* The parser seems to compain about Invalid IPv6 URL?

I openend the mail in Apple Mail to take a look.

* The links that reads ok in APple Mail seems to work just fine.
* But I find text that Apple Mails seems to interpret as an URL but it is not?

```text
“.*” - matches everything (‘.’ matches any character and ‘*’ means zero-or-more)
	“\\[^\\]+$” - Matches “\Hello.exe”
```

* For some reason Apple mail seems to interpret ```\\[^\\``` as an URL?

Can we implement a fallback for such URL:s and produce some viable html then markdown for it?

I asked Claude to vibe code me a solution.

```sh
Both issues fixed.

1. Garbled log ordering (emls_to_chimes.py): when stdout and stderr are both redirected to the same file (> output.log 2>&1), stdout was block-buffered while stderr wasn't — so a FAIL: line (stderr) could land mid-way through a not-yet-flushed stdout line, producing exactly ...chime.mdFAIL: .... Fixed by forcing line buffering on both streams at the top of the batch driver.

2. ValueError: Invalid IPv6 URL (html_to_markdown.py): Apple Mail's data-detector had turned a regex snippet (\[^\]+$) into an href shaped like //[^\]+$ — Python's urlsplit() reads a leading //[ as a broken IPv6 host literal and raises. Root cause: is_local_ref() called urlsplit() unguarded. Fix:
- New url_parts() wraps urlsplit() and returns None on a ValueError instead of crashing.
- is_local_ref() now treats an unparsable URL as "not local" rather than raising.
- Replaced the ad-hoc autolink condition (from last session's fix) with a proper is_autolinkable() — a link only becomes a bare <url> autolink when it's well-formed and has a URI scheme, which is what CommonMark autolinks actually require. Malformed or schemeless hrefs now fall through to ordinary [label](target) link syntax instead.

Verified with a synthetic reproduction of your exact mail shape (//[^\]+$ href) through the full eml_to_chime pipeline — no crash, produces [//\[^\\]+$](//[^\]+$). Added test_html_to_markdown.py (3 tests: malformed href doesn't crash, external URL still autolinks, local ref with matching label gets bracket-link form) — all 13 tests across the suite pass, and the 5-mail batch in this session still runs clean (5 ok, 0 failed).
```

This does not seem to be exactly what I want?

* I would like the faulty URL to be treated as plain text?
* At least for the example mail this is the correct move?

Anyways, I decide when I have tried the reafctored mechaism on my Todo-mails again.

## 20260929

This is take 2 on [Consider ways to transform a mail eml-file into a chime folder with chime.md and image files?](../71388c86/session.md)

* We go back to a pipe-line eml-to-html, html-to-markdown and then markdown-to-chime
* And we want the end result emls-to-chimes calling eml-to-chime that in turn does the 'chime' init and then html-to-chime into the chime?

I imagine we can clone the relevant scripts from previous session '../71388c86/' and refactor them into the mechanism we would like to have?

I vide coded the mechanism.

* eml_to_chime.py
* eml_to_html.py
* emls_to_chimes.py
* html_to_markdown.py
* test_eml_to_chime.py

Using a cloned 'init_new.py' to be able to apply the git repo root 'in it new artcicle' mechanism.

Now let's see what Jekyll produces on the chimes we get from the eml-files?

* We may be able to apply the mechanism in 'session/805fef21/'?
* Or maybe we should go back to session/805fef21/ and clone our chimes there for testing?

I asked Claude to perform the test.

* It seesm to work
* But it littered this session with the to_site python scripts!

Anyways, good enough for now I think (fingers crossed)!



