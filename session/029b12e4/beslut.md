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
* **cratchit verkar utan att märkas.** cratchit flyttar, döper om, ändrar och tar aldrig bort användarens egna filer. cratchit skriver i sina egna `.cratchit`-mappar, och skapar synliga filer och mappar bara när användaren ber om just det (en export till SIE; en ny bokföring får bara en `.cratchit`-mapp, se 2026-11-16). Ersätter beslutet att SIE-filen flyttas till `importerade_sie_filer`. Gamla `importerade_sie_filer`-mappar lämnas orörda.
* **Förteckning** (i koden `Inventory`) är cratchits uppsikt över bokföringens mapp: de filer bokföringen har fått något från, kända på SHA-256 av innehållet (egen implementering, `core/sha256`, testad mot NIST). Första versionen gäller SIE-filer som läses in.
  * Två nivåer: `.cratchit/inventory.csv` för hela bokföringen och `.cratchit/books/<årets start>/inventory.csv` per räkenskapsår, där årets poster ersätter bokföringens för samma fil (användarens val: mappen kan vara olika ordnad över åren). En SIE-fil hamnar i förteckningen för sitt räkenskapsår (`#RAR 0`).
  * CSV med rubrikrad, en rad per inläsning: `sha256, kind, name, path, size, read, use, generated, program, year_start, year_end, status, vouchers_added, vouchers_changed`.
  * `path` är relativ till bokföringens mapp, också för filer utanför den (användarens val: hela mappen ska kunna flyttas till en annan plats och dator).
  * En post skrivs bara när en fil används (ny bokföring, import), inte när Tab listar filer. Förteckningen fylls inte i i efterhand.
  * SHA-värdet visas aldrig i gränssnittet (användarens val; det hör hemma i ett administratörsgränssnitt som inte görs nu). Det får stå i loggen.
  * Ingen märkning `inläst` i Tab-listan och ingen rad om källan i räkenskapsåret för nu (användarens val).
* **Importera från SIE jämför filen med bokföringen.** *Bekräfta* visar alltid jämförelsen serie för serie (hur långt bokföringen och filen räcker, de nya verifikationerna som intervall), och sist om importen går att göra eller varför inte, med de verifikationer i bokföringen som filen skulle ta bort eller byta ut. Valen är `J Ja`/`N Nej`, eller bara `N Tillbaka` när importen inte går; `N` går tillbaka till mappen där filen valdes. Fel under prompten bara när ingen jämförelse går att göra (ingen SIE-fil, fel år, ett nytt år som inte ansluter).
* **Inga luckor.** Ett nytt räkenskapsår måste ansluta till bokföringen (börja dagen efter det senaste årets slut eller sluta dagen före det äldstas början). En serie i filen får inte ha luckor i numren; eftersom filen är hela året ansluter då de nya verifikationerna direkt efter bokföringens.
* **Ersätt året med SIE-filens innehåll** är utvägen när en import inte går på grund av konflikter: "bränn broarna och gör om" (användarens beslut). Ersätter förslagen om ögonblicksbilder och ett eget val *Ersätt året från SIE*; processen bygger bara på SIE-filer.
  * Importens `Bekräfta` säger då `Importen går inte att göra. <orsak>` och frågar `Ersätt året med SIE-filens innehåll?` med `J`/`N`. `J` leder till `Ersätt`, en rapport över vad som försvinner och kommer in, med `J`/`N`. `N` går tillbaka till att välja fil, `-` ett steg.
  * Erbjuds bara när verifikationer i vägen (saknas i filen, annat innehåll, annat datum eller annan text) är enda hindret. Aldrig vid annat organisationsnummer, luckor i en serie, eller när allt redan finns.
  * Året blir exakt filens. Det som skilde sig är borta: ingen kopia, inga utkast av det som försvann, ingen påminnelse om export. Årets utkast ligger kvar utom de filen bokför. Ett stängt år ersätts ändå och förblir stängt, med en varning.
  * Spårbarhet: det som försvann loggas med alla rader, och filen förs in i förteckningen som `ersätt`. Räkenskapsåret visar efteråt vad som försvann, med uppmaningen att bokföra det igen från underlag eller kontoutdrag.
* **cratchit hanterar för närvarande det aktuella räkenskapsåret och högst 9 äldre** (användarens beslut, en begränsning tills vidare; `max_older_fiscal_years` i `core/fiscal_year`). Listan visar tio år, `0`–`9`, med det tomma året före det äldsta inräknat. *Från uppgifter* vägrar ett äldre första år; *Från SIE* varnar i `Bekräfta` att året inte kommer att synas, men skapar bokföringen med `J`. Att bygga om en bokföring från SIE-filer görs från det senaste året bakåt.

## 2026-11-16

* **Editorn för verifikationer och utkast** byggs nu (användarens beslut). Första steget är en bläddringsbar lista över de bokförda verifikationerna i *Årsflöde > 1 Bokfört*.
* **Bläddringsbar lista** (användarens val): en sida med fast längd, 20 rader till att börja med; `…` överst när listan inte är i början och nederst när den inte är i slutet (bara `…`, utan antal); `<` och `>` bläddrar en hel sida, som `more`, och syns bara när det går; nyast överst. Sidindelningen är allmän, `pyroceph::tui::Pager`.
* **Ett val som inte kan göra något just då gör ingenting** och börjar ingen text (användarens förenkling). `<` och `>` i en bläddringsbar lista visas därför alltid.
* **Ett verifikations-id är serie och nummer som ett ord**, till exempel `A12` och `D1` (användarens beslut: allmänt vedertaget). Så visas id:n i alla vyer, och så skriver användaren dem, utan mellanslag. Serien matchas exakt; numret jämförs som tal, så `D1` är aldrig `D11`. Verifikationens state heter sitt id i sökvägen.
* **Öppna en verifikation:** i *Bokfört* skriver användaren id:t och trycker Enter. Verifikationens state visar för nu bara verifikationen (datum, rubrik, belopp, registrerad, konteringar med kontonamn och märkningen `(struken)` och `(tillagd)`); det ska växa till editorn.
* **Verifikationsserier har namn i bokföringen**, i `books/series.csv` (`series, name`) bredvid kontoplanen (användarens val). SIE har inga namn på serier. Standard är Fortnox serier A–N med Fortnox namn (användarens lista från Fortnox), hårdkodade som default även för bokföringar utan filen. En serie som verifikationerna använder men som saknas läggs till utan namn. Verifikationens vy visar raden `Serie:` först, med namnet. Att namnge serier i cratchit (*Kontoplan > Serier*) kommer senare.
* **Ny bokföring från SIE skapas i SIE-filens egen mapp** (användarens beslut) från *Samlingsmapp > Lägg till > Ny > Från SIE*. *Ny bokföring här > Från SIE* skapar den som förut i arbetsmappen, var SIE-filen än ligger (användarens rättelse: "här" är där du står, annars där filen ligger). cratchit skapar ingen ny mapp, bara `.cratchit`; användaren skapar själv mappen eller återanvänder en som finns, till exempel från ett annat program. Ersätter "en ny mapp med företagets namn bredvid SIE-filen". Följder:
  * Företagsnamnet behöver inte gå att använda som mappnamn.
  * Samma kontroller som *Ny bokföring här* (`check_project_folder` i `core/details_project`): inte i en mapp som redan har `.cratchit`, inuti en annan bokföring, i hemkatalogen eller i samlingsmappen.
  * `Bekräfta` visar raden `Mapp:` med mappen bokföringen skapas i.
  * *Från uppgifter* i samlingsmappen skapar fortfarande en ny mapp med namnet i samlingsmappen (inte ändrat nu).
* **En mapp har en bokföring** (användarens beslut). När samlingsmappen kommer ihåg en bokföring glömmer den andra uuid som den har för samma mapp, och loggar det (`remember_project` i `core/project`).
