# Consider a two step mail eml to domain and then some domain to domain mechanism?

* [eml_to_domain/README.md](./eml_to_domain/README.md)
* [domain_to_domain/README.md](./domain_to_domain/README.md)

## 20261008

I decided to change how the date in a domain entry from a mail is presented in the text.

* It is now a readable text on the form 'As of yyyy-mm-dd hh:mm´
* The eml-to-domain-entry now creates and parses this date to have newest mail with same subject prevail ok.

I made Claude vibe code it and it seems ok.

I now made Claude rename the '--exclude' mechanism to '--ignore' ok.

* Claude applied the term 'IGNORE' across the board (which I think is ok?)

I now asked Claude to create a 'test_on_todo_mails.py' and it did.

* Great! Now I have it easier to remember the scripts to call to perform the whole pipe line as I design it. 

I now chatted with Claude about how to make the generated site 'nicer' to navigate.

* It suggested some godd enhancement.
* I went with adding links between entries in the same domain.
* I asked to name the mechanism 'update_domain intralink ...' [update_domain/README.md](./update_domain/README.md) 

I tested it on my large set of chimes from todo-mails and what I could see it works quite well?

## 20261007

* [site_repo/chime/index.md](./site_repo/chime/index.md)

I now added to_site from session/afaa732d.

* I updated site_repo/index.md to link to chimes index.
* I then generated the site with 'to_site.py site_repo' ok.

From what I can see all chimes I viewed in the browser matches the original mail ok?

* But I now want some info about where the chime originated.
* I chatted with Claude but did not get convinced by any design.
    * Claude proposed an elaborated front-matter
    * I hesitate about this (it is too convoluted and vague?)
    * I mean, I want some way to see where the domain entry came from

At this stage I came to think about all my 'Also see ...' that refers to other todo-mails.

* Maybe I should design a way to turn those into navigable links?
* This kind-of resembles ('based on', 'originates from')?
* Or at least is a reference mechanism in the same way as 'from-mail' is one?
* But the information that the entry started as a mail is a reference to something unreachable!
* While 'Also see' can be expected to be reachable (in the same domain even)

I have to think about this.

* I want a date that the reader can se that informs about when the entry state was 'created'
* And I want the 'Also see' links to be markdown (and then also html) links for inter-entry navigation.
* And, I would like to AVOID having machine-only meta-data in the mix!

If I let this marinate I can maybe come up with a feasible next step?

## 20261006

I have now asked claude to implement domain_to_domain init/add.

This seems promising.

* I now made domain_to_domain init allow for no --pick and then cerate an empty target domain.
* I also added exclude.md from session where we developed the first eml-to-chime mechanism.
    * My plan is to turn this into a pick list by comparing with source domain index.

After having discussed with Claude and thought about it I have some ideas on how to proceed.

* I want to be able to use an exclude.md for cases where what to pick far outweighs what to exclude.
* But I still want the domain_to_domain to only support opt-in for what to copy.
* One way could be to define a 'diff' mode that outputs 'pending' or 'orphans' in the source
* Question is how we can apply exclude.md?
    * It could be an argument to the diff mode?
    * A 'diff' with an --exclude listing could mean 'ignore source orphans in exclude listing'?

That seems promising?

* The user can first do 'domain_to_domain init ./site_repo/ mail chime' to ge an empty chime domain?
* Then do 'domain_to_domain diff ./site_repo/ mail chime' to see all in source but not in target?
* Then do 'domain_to_domain diff ./site_repo/ mail chime --exclude exclude.md' to see all in source but not in target but that is also not already excluded?
    * The result of this operation can be used as a pick list!
    * 'domain_to_domain diff ./site_repo/ mail chime --exclude exclude.md > pick.md'
    * 'domain_to_domain add ./site_repo/ mail chime --pick pick.md'

I chatted with Claude and Claude ended up implementing a diff mode.

I tried it out and it seems to work just fine?

```sh
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/plaiground/session/0dd1a71d % ./domain_to_domain/domain_to_domain.py init site_repo mail chime
Created empty target domain 'chime' (site_repo/chime)
Updated 'chime/index.md' with 0 entries.
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/plaiground/session/0dd1a71d % 

kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/plaiground/session/0dd1a71d % ./domain_to_domain/domain_to_domain.py diff site_repo mail chime > pick.md
mail -> chime (diff): 3204 pending, 0 excluded, 0 in target
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/plaiground/session/0dd1a71d % 

kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/plaiground/session/0dd1a71d % ./domain_to_domain/domain_to_domain.py diff site_repo mail chime --exclude exclude.md > pick.md 
mail -> chime (diff): 2942 pending, 262 excluded, 0 in target
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/plaiground/session/0dd1a71d % 

kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/plaiground/session/0dd1a71d % ./domain_to_domain/domain_to_domain.py add site_repo mail chime --pick pick.md
# ...
mail -> chime (add), 2942 pick entries: 2942 added, 0 skipped, 0 failed, 0 unmatched
Updated 'chime/index.md' with 2942 entries.
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/plaiground/session/0dd1a71d %

```

So far so good?

No. Our mechanism folders are littered with test files and scripts?

* I want a clean eml_to_domain to share with clients.

```sh
eml_to_domain
├── README.md
├── eml_to_domain.py
├── eml_to_html.py
├── eml_to_txt.py
├── emls_to_domain.py
└── html_to_markdown.py
```

* I want a clean 

```sh
domain_to_domain
├── README.md
└── domain_to_domain.py
```


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

* I quickly realized we now enter the complexity of 'synchronization'?
* Should the domain_to_domain be an update operation or a sync operation?
* An update operation allows for incremental runs to apply changes
    * But then the question is if removal from target should be allowed as an update?
    * Or if update should only allow for adding entries missing in target?
* A sync operation applies a harder control to ensure target is exactly as defined by the operation
    * For --include any entry in target not in the listed entries shall be REMOVED
    * And if we add --exclude, then target shall still be as defined by ALL - exclude.

It all gets tricky fast!

* For my current needs I can just implement something to get my chimes and publish on the web.
* But then I have problems later if or when I want to add more entries later?
    * How should domain_to_domain behave if I later want to add entries I did not add the first time?
    * It seems I kind-of want an incremental mechanism?
    * But one that protects any edits I do to the chimes already imported?

Come to think about it. The eml processing pipe already implements an incremental update for 'same entry' as in 'entry with same subject and latest date' is the one that prevails.

* So if I update a todo-mail I can import emls again and have the target update to the entry with the latest date.
* Problem is that domain_to_domain have no date to compare.
    * That is, the date in the markdown file is the mail date.
    * This date does not change if I edit the markdown later in the target domain.

GOSH!!

Anyhow, I now asked Claude to change how the date is written to the markdown for imported eml-files.

* It is now written as an html-comment ( E.g. ``` <!-- mail-date: 2015-12-19T18:08:05+01:00 --> ```)
* An eml-file with a date supersedes a markdown that does not have the 'new' date tagging.
* So I re-ran on my todo-mails and got all updated

```sh
# ...
4433 mail files -> mail: 0 added, 3691 updated, 742 skipped (superseded), 0 failed
Updated 'mail/index.md' with 3204 entries.
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/plaiground/session/0dd1a71d % 
```

This feels good for now.

* Now it is clear that the 'date' thing for entries that comes from emails.
* So we now have no date in the markdown to be treated as having anything to do with edits of the markdown.
* And the eml-file processing can still pick out the latest mail with the same subject.

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

I now realize I actually think the created markdown from eml file (email) should be without the hash tag?

* I can invent some UUID later?

I asked Claude to apply this change and it did.

I now added the example_eml from session/afaa732d to use in this one.

* The ```python3 ./eml_to_domain/emls_to_domain.py example_eml site_repo --domain mail``` now creates [site_repo/mail/index.md](./site_repo/mail/index.md) ok.

So we are back at markdowns without any hash ok.