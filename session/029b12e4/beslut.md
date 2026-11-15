# Beslut

Beslut som gäller i den här sessionen. Beslut från tidigare sessioner gäller inte här förrän de står i den här filen. Källorna att hämta dem från står i [status.md](status.md#källor).

## 2026-11-15

* Sessionen utgår från koden i 0a37da4b (`cratchit/`, git-spårade filer, steg 77). Den är kopierad hit oförändrad.
* `beslut.md` och `status.md` börjar om från början. Användaren och Claude tar in tidigare beslut och status en i taget, när arbetet behöver dem.
* Målet: en cratchit som går att använda på riktigt. De verifierade delarna från andra sessioner (årsredovisning, INK2, momsrapport, PDF till text) ska bli en del av den.
* cratchit installeras för hela datorn, på cmakes standardplats för plattformen (`/usr/local` på macOS och Linux, `C:\Program Files\cratchit` på Windows). Användarens skäl: ingen delar sin arbetsdator i dag. Användarens egna filer (`~/.cratchit`, samlingsmappen) är fortfarande per användare.
* `install.py` är borttaget för nu. Installationen sker med cmake (se `cratchit/README.md`).
* En bokföring kan skapas i arbetsmappen själv (mappen cratchit startas i), i dess `.cratchit/`, i stället för i en ny mapp i samlingsmappen. Det är för den riktiga bokföringen i företagets befintliga mapp. Valet är `1 Ny bokföring här` i roten och syns bara när arbetsmappen saknar bokföring; `0` är fortfarande `Samlingsmapp` (användarens val). cratchit vägrar i en mapp som redan har `.cratchit`, i hemkatalogen, i samlingsmappen och inuti en annan bokföring.
* Förslaget om testbokföringar som går att börja om från ett SIE-frö (med en rad `test = <SIE-fil>` i `project.txt`) är inte beslutat. Användaren återkommer.
* En SIE-fils räkenskapsår (`#RAR 0`) har en status, räknad från dagens datum (användarens begrepp, adjektiv om räkenskapsåret):
  * **pågående**: året har inte slutat i dag.
  * **ofullbordat**: året är slut, men balansräkningen (konton 1xxx–2xxx, `#IB 0` plus raderna) balanserar inte och inget är bokfört på 8999. Är året äldre än förra året (slutade för mer än ett år sedan) varnar cratchit, för ett sådant år ska vara fullbordat.
  * **fullbordat**: balansräkningen balanserar, och de kontroller som läggs till senare visar ok.
  * **misstämmer**: något är bokfört på 8999, men balansräkningen balanserar inte (eller, senare, en kontroll visar fel).
  * Statusen är information. cratchit hindrar inte att användaren väljer en fil med någon status. *Fullbordat* betyder inte inlämnad årsredovisning eller låst år.
  * "Oriktigt" valdes bort: *oriktig uppgift* är ett skatterättsligt begrepp.
