# Consider a two step mail eml to domain and then some domain to domain mechanism?

## 20261005

Time to try the eml processing on my todo mails.

```sh
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/plaiground/session/0dd1a71d % ./eml_to_domain/emls_to_domain.py ~/Downloads/mail_export/eml site_repo --domain mail
# ...
4433 mail files -> mail: 3196 added, 485 updated, 752 skipped (superseded), 0 failed
Updated 'mail/index.md' with 3204 entries.
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/plaiground/session/0dd1a71d %
```

* [site_repo/mail/index](./site_repo/mail/index.md)

I now chatted with Claude about adding an --exclude option too?

* I quickly realised we now enter the complexity of 'synchronization'?
* Should the domain_to_domain be an update operation or a sync operation?
* An update operation allows for incremental runs to apply changes
    * But then the question is if removal from target should be allowed as an update?
    * Or if update should only allow for adding entries missing in target?
* A sync operation applies a harder control to ensure target is exactly as defined by the operation
    * For --include any entry in target not in the listed entries shall be REMOVED
    * And if we add --exclude, then target shall still be as defined by ALL - exclude.

It all gets trycky fast!

* For my current needs I can just implement something to get my chimes and publish on the web.
* But then I have problems later if or when I want to add more entries later?
    * How should domain_to_domain behave if I later whant to add entries I did not add the first time?
    * It seems I kind-of want an incremental mechanism?
    * But one that protects any edits I do to the chimes already imported?

Come to think about it. The eml processing pipe already implements an incremental update for 'same entry' as in 'entry with same subject and latest date' is the one that prevails.

* So if I update a todo-mail I can import emls again and have the target update to the entry with the latest date.
* Problem is that domain_to_domain have no date to compare.
    * That is, the date in the marldown file is the mail date.
    * This date does not change if I edit the makrdown later in the target domain.

GOSH!!

## 20261004

So I have [Consider to vibe code a working emls-to-chimes and to_site integration?](../afaa732d/session.md) that now works seemingly well.

But I want the process of turning eml files into 'chimes' to be broken up into different steps.

1. First import ALL emls into markdowns named after some provided domain (NOT hard coded to 'chime')
2. Then use a separate mechanism to pick from the index of the domain markdowns into another domain
3. Have the init_new.py script that initiates a new markdown inject the FULL hash into the markdown document.

I want Claude to clone relevant code from [Consider to vibe code a working emls-to-chimes and to_site integration?](../afaa732d/session.md) and create new scripts and files in this session.

Let's see what Claude can help us with.

So Claude seems to have done a good job of creating the new eml to 'domain' mechanism.

* [eml_to_domain/README.md](./eml_to_domain/README.md)

I now realise I actually think the created markdown from eml file (email) should be without the hash tag?

* I can invent some UUID later?

I askeed Claude to apply this change and it did.

I now added the example_eml from session/afaa732d to use in this one.

* The ```python3 ./eml_to_domain/emls_to_domain.py example_eml site_repo --domain mail``` now creates [site_repo/mail/index.md](./site_repo/mail/index.md) ok.

So we are back at markdowns without any hash ok.