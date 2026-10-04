# Consider a two step mail eml to domain and then some domain to domain mechanism?

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