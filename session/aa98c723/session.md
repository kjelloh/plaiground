# Consider take 2 on eml-to-chime python mechanism?

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



