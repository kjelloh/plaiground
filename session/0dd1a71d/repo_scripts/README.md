# repo_scripts — scripts that live at the root of a target repo

Copy these into the root of a target repo (e.g. [site_repo/](../site_repo/)).
`eml_to_domain` and `domain_to_domain` call whichever copies the target repo holds.

* `init_new.py <domain> 'Heading'` creates `<domain>/<hash>/<domain>.md` (unchanged copy).
* `update_index.py <domain>` writes `<domain>/index.md`, one link per entry.

## Index order

Entries are sorted on their heading as-is, i.e. Unicode code point order
(the same as UTF-8 byte order), not on the hash folder name:

* uppercase before lowercase (`TODO:` < `Todo:` < `todo:`)
* `Ä` < `Å` < `Ö` < `ä` < `å` < `ö`, all after `z` (not Swedish order)

Entries with the same heading are kept in hash order, so the index is the
same on every run and every machine.

## Tests

```sh
python3 -m pytest          # in repo_scripts/, needs pytest
```
