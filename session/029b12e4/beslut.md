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
* **Tangenter matchas exakt.** Ett val som visas som `J` nås bara med `J`, inte med `j` (användarens regel: enkel kod). cratchit gör undantag bara när bekvämligheten inte gör det otydligare vad tangenten gör, och undantaget skrivs in här. Inga undantag än.
* **cratchit verkar utan att märkas.** cratchit flyttar, döper om, ändrar och tar aldrig bort användarens egna filer. cratchit skriver i sina egna `.cratchit`-mappar, och skapar synliga filer och mappar bara när användaren ber om just det (en ny bokföring i en ny mapp, en export till SIE). Ersätter beslutet att SIE-filen flyttas till `importerade_sie_filer`. Gamla `importerade_sie_filer`-mappar lämnas orörda.
* **Förteckning** (i koden `Inventory`) är cratchits uppsikt över bokföringens mapp: de filer bokföringen har fått något från, kända på SHA-256 av innehållet (egen implementering, `core/sha256`, testad mot NIST). Första versionen gäller SIE-filer som läses in.
  * Två nivåer: `.cratchit/inventory.csv` för hela bokföringen och `.cratchit/books/<årets start>/inventory.csv` per räkenskapsår, där årets poster ersätter bokföringens för samma fil (användarens val: mappen kan vara olika ordnad över åren). En SIE-fil hamnar i förteckningen för sitt räkenskapsår (`#RAR 0`).
  * CSV med rubrikrad, en rad per inläsning: `sha256, kind, name, path, size, read, use, generated, program, year_start, year_end, status, vouchers_added, vouchers_changed`.
  * `path` är relativ till bokföringens mapp, också för filer utanför den (användarens val: hela mappen ska kunna flyttas till en annan plats och dator).
  * En post skrivs bara när en fil används (ny bokföring, import), inte när Tab listar filer. Förteckningen fylls inte i i efterhand.
  * SHA-värdet visas aldrig i gränssnittet (användarens val; det hör hemma i ett administratörsgränssnitt som inte görs nu). Det får stå i loggen.
  * Ingen märkning `inläst` i Tab-listan och ingen rad om källan i räkenskapsåret för nu (användarens val).
* **Importera från SIE jämför filen med bokföringen.** *Bekräfta* visar alltid jämförelsen serie för serie (hur långt bokföringen och filen räcker, de nya verifikationerna som intervall), och sist om importen går att göra eller varför inte, med de verifikationer i bokföringen som filen skulle ta bort eller byta ut. Valen är `J Ja`/`N Nej`, eller bara `N Tillbaka` när importen inte går; `N` går tillbaka till mappen där filen valdes. Fel under prompten bara när ingen jämförelse går att göra (ingen SIE-fil, fel år, ett nytt år som inte ansluter).
* **Inga luckor.** Ett nytt räkenskapsår måste ansluta till bokföringen (börja dagen efter det senaste årets slut eller sluta dagen före det äldstas början). En serie i filen får inte ha luckor i numren; eftersom filen är hela året ansluter då de nya verifikationerna direkt efter bokföringens.
