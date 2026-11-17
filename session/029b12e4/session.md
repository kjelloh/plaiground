# Överväg att knyta ihop säcken och skapa ett produktionsredo cratchit bokföringsprogram från framgångsrika sessioner?

```#029b12e4```

## Common

* Vi utgår från koden vi tog fram i session/0a37da4b (../0a37da4b)  
    * Den har vårt nya UI (pyroceph och TUI)
    * Den kod vi redan har i denna session är koden från session/0a37da4b
    * För detaljer se [Överväg att vibe-koda cratchit bokslut på svenska från senaste cratchit-sessioner](../0a37da4b/session.md)
* Vi har en grundstomme till cratchit bokföring i session/60cae9fe (../60cae9fe)
    * Den har funktioner för bokföring och en datamodell vi kan inspireras av
    * För detaljer se [Vibe code a Swedish book keeping app named cratchit - take 3 (Drop feed Claude)](../60cae9fe/session.md)
* Vi har cratchit_ar i session/5b35b2b9 '../5b35b2b9'
    * Den tar en SIE-fil och skapar underlag för en årsredivsining
    * Den är verifierad mot tidigare årsredovisningar samt ett utkast från Årsredovisning Online för årets bokföring ok
    * För detaljer se [# Consider a cratchit app that maps an SIE file to a Swedish Årsredovisning?](../5b35b2b9/session.md)
* Vi har cratchit_ink2 i session/7d4475fb '../7d4475fb'
    * Den tar en SIE-file och genererar ett underlag för inkomstdeklaration 2 (INK2)
    * Den är  verifierad mot tidigare SRU-filer ok
    * Den är nu använda för att generera årets deklaration.
    * För detaljer se [Consider a cratchit app that maps an SIE file to a Swedish SKV Ink_2?](../7d4475fb/session.md)
* Vi har en app walk_pdf i session/9fe36d28 (../9fe36d28)
    * Den producerar ett underlag som visar strukturen för en pdf 
    * Den verkar kapabel att skapa klartext från en pdf (inklusive avkoda en ebook till klartext)
    * [Consider to vibe code a pdf-walker C++ console app?](../9fe36d28/session.md)

* Bygg med: cmake --build cratchit/build/Debug -j
* Installera med: sudo cmake --install cratchit/build/Debug

## 20261117

Så har jag till slut preppat så jag kan testa att återskapa bokföring från några kända tillstånd.

* Jag har itfied_test som mitt bas-project

    * Och jag har skapat bokföringar för senast kända tillstånd
    * För varje tillstånd har jag kopierat .cratchit-mappen till __snapshot

    ```sh
    kjell-olovhogdahl@MacBook-Pro ~/Documents/cratchit % tree -a -L 2
    .
    ├── .DS_Store
    ├── .cratchit
    │   └── workspace.txt
    ├── __snapshot
    │   ├── .cratchit_20260430_innan_bokslut
    │   ├── .cratchit_20260630_efter_M1
    │   ├── .cratchit_20261012_efter_M2
    │   └── .cratchit_20261012_efter_bokslut
    └── itfied_test
        └── .cratchit

    9 directories, 2 files
    kjell-olovhogdahl@MacBook-Pro ~/Documents/cratchit % 
    ```

    * Så för att återställa ett tillstånd ersätter jag .cratchit med lämplig .cratchit-mapp i __snapshot

Kvar från igår har jag också ett första utkast till att editera en verifikation.

* Jag vet inte om det kommer att fungera.
* Det blir ganska rörigt?
* Men OK, jag köper det för nu så får vi se om vi kan få det att funka?

Jag har bestämt nu att bygga upp cratchit från 'andra hållet'

* Göra om bokföring för M2
* Göra om momsrapporten M2
* Göra om bokslutet 20250501-202060430
* Göra om Årsredovisning 20250501-202060430
* Göra om Stämman för 20250501-202060430
* Göra om INK2 för 20250501-202060430

## 20261116

Nu är det väl ändå dags att till slut få till ett sätt att redigera och skapa verifikationer till bokföringen?

* Jag har hittills av någon anledning inte gjort en bra sådan!
* Varför? Jag vet inte! Det är som om problemet flyr mig hela tiden! 
* Eller så är det så att jag inte gillar hur dubbel bokföring på BAS-konton gör det så ogenomskinligt och krångligt?

Men så, hur tacklar vi detta problem. Kanske kan vi börja med vad jag tror jag 'vet'?

* En verifikation skall bokföra en affärshändelse.
* Den har en rubrik, ett datum och ett belopp.
* Beloppet skall fördelas på konton och motkonton till lika delar debit som kredit.
* I Sverige används nästan alltid BAS-kontoplanen.
* Så beloppet konteras på olika BAS-konton.
* En 'rad' konterar ett delbelopp på ett BAS-konto med en frivillig text.
* Summan av alla konteringar skall vara noll (lika mycket debit som kredit)
* BAS-kontoplanen definierar regler för vilka konton som skall användas för olika affärshändelser (finansiella händelser?).

Till detta kommer bokföringslagen som reglerar vad man måste och eller får göra i och med sin bokföring.

Jag tror jag vet vad som räcker till att börja med?

* En händelse som är bokförd får i princip inte ändras.
* Men det verkar som om man får stryka rader.
    * Men en strykt rad måste stå kvar i verifikationen för spårbarhet.
    * SIE-filformatet har stöd för detta (Se tagg BTRANS)
* Det verkar också som om man får lägga till rader.
    * Men man bör (måste?) markera att raden har lagts till (inte var där i den ursprungliga bokförda verifikationen)?
    * SIE-filformatet har stöd för det också (Se tagg RTRANS)
* Dock verkar SIE-filformatet inte kräva att strukna och tillagda rader kan tas emot av ett program som läser in SIE-filer?
    * Se officiella [SIE_filformat_ver_4C_2025-08-06](https://sie.se/wp-content/uploads/2026/02/SIE_filformat_ver_4C_2025-08-06.pdf)

    ```text
    Hanteras inte poster av typen #BTRANS och #RTRANS skall dessa rader ignoreras vid
    import av en SIE-fil.
    ```
    * Så det verkar som om en strykt rad BARA står som en BTRANS (ursprungliga TRANS är ersatt av en BTRANS?)
    * Då blir det OK för mottagaren att ignorera BTRANS (Den är ju 'borttagen')
    * PÅ samma sätt måste en tillagd rad BÅDE markeras med en rad RTRANS OCH en vanlig TRANS!
    * PÅ så sätt kan mottagaren ignorera RTRANS och ändå läsa in vanlig TRANS och få med rätt konteringar.

OK, så här tänker jag nu för Cratchit.

* Vi har delat upp bokföringen i 'Bokförda' och 'Utkast'
* En editor för bokförda verifikationer skall begränsa vad användaren kan göra
    * Man skall kunna stryka och lägga till rader
    * Frågan är om man skall kunna ändra i rubriken?
    * Frågan är också om man skall kunna ändra datum?
    * I Fortnox-programmet får man det (Så då kanske det är enklast om vi tillåter det också?)
    * Ett enkelt UI kan vara ett första val mellan 'sammanfattning' och 'konteringar'?
    * Och alternativen för 'sammanfattning' är 'rubrik','belopp' och 'datum'?
    * Och alternativen för konteringar är en numrerad lista 0..9 av konteringar?
    * Alternativen för en vald kontering kan vara numrerat 'konto', 'belopp', 'beskrivning'
    * Men så skall kontering också kunna strykas, vilket kanske kan ske med en dold bokstavs-val 'b' för 'borttagen'?
    * En grundregel för editorn kan vara att 'historiken måste bevaras'?
    * Bör man kunna välja att göra om en vald verifikation till utkast?
    * Det krånglar ju till det lite då endast den sista i en serie kan göras om till utkast?
    * Men det känns samtidigt naturligt att ha detta val för den verifikation man valt?
* Jag tänker mig att en bokförd verifikation skall kunna flyttas till 'utkast'
    * Endast den senaste i en serie skall gå att flytta till utkast
    * En verifikation skall behålla sin serie när den blir utkast
    * Frågan är om valet att flytta skall vara under 'bokförda' eller 'utkast'?
    * Under bokförda kanske man kan välja 'Flytta till Utkast' och sedan välja från vilken serie man vill flytta från (underförstått att man ändå bara kan välja den sista i en serie)?
    * Eller så finns ett val under 'Utkast' (men jag kommer inte på vad ett sådant val skulle kallas? Funkar 'Utkast från bokfört'?)
    * I så fall blir undervalet nog vilken serie (underförstått att man bara kan göra om sista bokförda verifikationen i en serie till utkast?)
    * Även här tycker jag ett utkast från tidigare bokförd verifikation behåller sin serie.
    * Men att man i utkastet kan ange vilken serie den skall bokföras på?
* Men vi kan tillåta editering av Utkast mycket friare till dess de blir bokförda.
    * Man skall INTE kunna ändra på konteringar som kommer från kontoutdrag.
    * Man skall INTE kunna ändra på sammanfattningar som kommer från underlag (inlästa dokument som beskriver affärshändelser, alltså elektroniska verifikat)
    * Man skall kunna slå samman utkast till ett utkast med de sammanslagna utkastens konteringar (för att få ett utkast som beskriver 'samma' händelse, till exempel ett kvitto och en kontering från ett kontoutdrag)
    * En editor för ett utkast skall lika editorn för en bokförd verifikation så långt det är möjligt (bara fler element som går att ändra)

OK, jag börjar förstå varför det tagit mig sådan tid att designa en editor för verifikationer. Det är KOMPLEXT!

Jag tror jag redan bestämt mig för ett flöde baserat på underlag och konteringar som sätts samman till utkast som blir bokförda verifikationer?

* Från kontoutdrag får vi konteringar och eventuella saldo-angivelser.
    * Vid inläsning av kontoutdrag väljer användaren vilket BAS-konto som skall spegla kontoutdragets angivelser.
    * Cratchit kan på så sätt spara enskilda konteringar. Men också eventuella saldo-angivelser.
    * Konteringar kan direkt sparas som utkast med en ännu ej angiven sammanfattning.
    * Saldo-angivelser kan sparas separat för avstämningar.
* Med kod från pdf_walker kan cratchit läsa in affärshändelser från pdf-er
    * pdf-er med endast grafik (t.ex., skannade underlag) kan cratchit spara som 'källa' för att ändå hålla samman en verifikation med ett elektroniskt dokument.
    * En pdf med text kan cratchit tolka till ett utkast med rubrik, belopp och datum.

OK, börjar vi få lite kött på benen? Var börjar vi för att försöka få allt på plats utan att glömma något eller skapa ett rörigt UI?

I vilken ända börjar vi?

* Skall vi först göra stommen till en editor av en verifikation?

```text
    Utkast/Verifikation
    ID Datum Rubrik Belopp
        Konto Beskrivning Belopp Konto-namn
        Konto Beskrivning Belopp Konto-namn
        Konto Beskrivning Belopp Konto-namn
        ...
    0 Sammanfattning
    1 Konteringar

    Sammanfattning
    0 Serie
    1 Datum
    2 Rubrik
    3 Belopp

    Konteringar
    0 Konto Beskrivning Belopp Konto-namn
    1 Konto Beskrivning Belopp Konto-namn
    2 Konto Beskrivning Belopp Konto-namn
    ...
    Dolt
    + Lägg till kontering

    Kontering
    0 Konto
    1 Beskrivning
    2 Belopp
    Dolt
    b stryk (ta bort)

```

* Eller skall vi börja med att ta in koden som läser in kontoutdrag?
    * Den koden finns redan i take 3 tror jag?
* Eller skall vi börja med inläsning av elektroniska underlag (som ännu inte finns)?
    * Vi har ingen kod som tolkar text i pdf och skapar utkast.
    * Är detta 'good to have' och kan vänta?
    * Eller skall vi i varje fall kunna länka utkast till filer?

Jag tror det nog är dags ändå att faktiskt göra ett UI för att editera en bokförd Verifikation och/eller ännu ej bokfört Utkast?

* Vi borde kunna använda 'samma' editor?
* Bara med mindre begränsningar för utkast?

Jag har nu börjat chatta med Claude och tror jag kan guida den fram till ett UI och beteende som är gott nog?

* Jag får se till att behålla fokus!
    * Målet är att kunna bläddra, välja och editera!

Vilka egna tester (användarfall) kan jag tänka mig att verifiera mot?

Nu uppkom behovet av att skriva ut namn på serier. Jag kopierade namnen från Fortnox (lista under Export SEI-fil)

```sh
 	A - Redovisning	
 	B - Kundfakturor	
 	C - Inbetalningar från kunder	
 	D - Leverantörsfakturor	
 	E - Utbetalningar till leverantörer	
 	F - Kassa	
 	G - Avskrivning	
 	H - Periodisering	
 	I - Bokslut	
 	J - Kontantfaktura	
 	K - Kvitto	
 	L - Lön	
 	M - Momsrapport	
 	N - Lager	
```

Frågan är om jag ändå blev lockad in på ett sidospår nu?

* Vi behöver ju inte seriernas namn för att editera en verifikation?
* Eller är det kanske en förutsättning (så man väljer rätt serie)?

Ja, nu blev det infört i alla fall (Tänk att ALLT är så komplext och bara växer och växer hela tiden?)

* HÅLL I DET HÄR NU!
* Jag vill kunna lämna det här och ha ett bokföringsprogram som är good enough fram till nästa bokslut, ÅR, Stämma, rapport till Bolagsverket och inkomstdeklaration 2!

Nu började jag skapa mall-företag för att kunna provköra bokföring från ett känst start-tillstånd.

* Jag märker då att 'skapa här' och 'ny från SIE' beter sig olika!
* Vi har gjort 'skapa här' så den faktiskt gör 'här' (inte i en undermapp)
* Medan 'ny från SIE' fortfarande skapar en undermapp döpt som företaget
* Jag tror detta är förvirrande?
    * Jag blev förvånad i alla fall!
    * Jag hade skapat mig bilden att 'ny frå SIE' nu alltid skapade i samma mapp som där SIE-filen låg?
    * Men så är så klart inte fallet.
    * Claude saknar förmågan att beakta den typen av meta-koncept (antar jag)?

I now tested a prompt that Claude took totally the wrong way.

```text
❯ Jag prövade 'Cratchit > Samlingsmapp > Lägg till > Ny > Från SIE' och märker nu att de skapar företaget i en undermapp till SIE-filen. Detta skiljer sig från 'Skapa här > från SIE' som skapar bokföingen i samma mapp som SIE-filen. Jag vill att alla 'ny fron SIE' skapar bokföringen i den mapp där SIE-filen ligger. Så det blir upp till användaren att skapa sin egen mapp (eller återanvända en befintlig från ett annat program). Kan du göra så alla 'ny från SIE' skapar en ny bokföring i samma mapp som SIE-filen ligger i? Kolla också om det blir problem om vi gör så? För och nackdelar? Vad kan du föreslå?
```

* Claude took this to mean 'always create new books in the same folder as the SIE-file.
* But this was NOT what I meant.
* But I now see I actually expressed my intention the wrong way.
    * What I wanted was 'never create sub-folder'"
    * I wanted 'create here' to create 'here' (current folder)
    * And 'new from SIE' in the same folder as the SIE-file

I asked Claude to correct this.

Nu har jag skapat tree varainetr av itifed i olika tillstånd så jag kan testa att återskapa bokföring med nya UI.

```sh
kjell-olovhogdahl@MacBook-Pro ~/Documents/cratchit % tree
.
├── __snapshot
│   ├── SIE-filer
│   │   ├── TheITfiedAB20261019_20250501_20260430.se
│   │   ├── TheITfiedAB20261019_20260501_20270430.se
│   │   ├── TheITfiedAB20261112_102636.se
│   │   └── TheITfiedAB20261112_111418.se
│   ├── itfied_20260430_efter_bokslut.cratchit
│   │   ├── books
│   │   │   ├── 2025-05-01
│   │   │   │   ├── inventory.csv
│   │   │   │   ├── opening.csv
│   │   │   │   ├── postings.csv
│   │   │   │   └── vouchers.csv
│   │   │   ├── accounts.csv
│   │   │   ├── books.txt
│   │   │   ├── dimensions.csv
│   │   │   ├── objects.csv
│   │   │   └── series.csv
│   │   └── project.txt
│   ├── itfied_20260430_innan_bokslut.cratchit
│   │   ├── books
│   │   │   ├── 2025-05-01
│   │   │   │   ├── inventory.csv
│   │   │   │   ├── opening.csv
│   │   │   │   ├── postings.csv
│   │   │   │   └── vouchers.csv
│   │   │   ├── accounts.csv
│   │   │   ├── books.txt
│   │   │   ├── dimensions.csv
│   │   │   ├── objects.csv
│   │   │   └── series.csv
│   │   └── project.txt
│   ├── itfied_2026_innan_M2.cratchit
│   │   ├── books
│   │   │   ├── 2025-05-01
│   │   │   │   ├── inventory.csv
│   │   │   │   ├── opening.csv
│   │   │   │   ├── postings.csv
│   │   │   │   └── vouchers.csv
│   │   │   ├── 2026-05-01
│   │   │   │   ├── inventory.csv
│   │   │   │   ├── opening.csv
│   │   │   │   ├── postings.csv
│   │   │   │   └── vouchers.csv
│   │   │   ├── accounts.csv
│   │   │   ├── books.txt
│   │   │   ├── dimensions.csv
│   │   │   ├── objects.csv
│   │   │   └── series.csv
│   │   └── project.txt
│   └── återställ.sh
├── itfied_20260430_efter_bokslut
├── itfied_20260430_innan_bokslut
└── itfied_2026_innan_M2

16 directories, 39 files
kjell-olovhogdahl@MacBook-Pro ~/Documents/cratchit % 
```

* Där itfied_20260430_efter_bokslut är 20250501-20260430 avslutat
* Där itfied_20260430_innan_bokslut är 20250501-20260430 innan bokslut
* Där itfied_2026_innan_M2 är 20260501-20270430 inna M2 är gjord
* I __snapshot ligger .cratchit-filer jag kan kopira tillbaka och återställa tillståndet
    * itfied_20260430_efter_bokslut.cratchit
    * itfied_20260430_innan_bokslut.cratchit
    * itfied_2026_innan_M2.cratchit
*  SIE-filerna i __snapshot/SIE-filer

┌──────────────────────────────────────────┬─────────┬─────────────────────────────────┐
│                   Fil                    │   År    │           Anmärkning            │
├──────────────────────────────────────────┼─────────┼─────────────────────────────────┤
│ TheITfiedAB20261019_20250501_20260430.se │ 2025/26 │ innan bokslut                   │
├──────────────────────────────────────────┼─────────┼─────────────────────────────────┤
│ TheITfiedAB20261112_102636.se            │ 2025/26 │ efter bokslut                   │
├──────────────────────────────────────────┼─────────┼─────────────────────────────────┤
│ TheITfiedAB20261112_111418.se            │ 2024/25 │ året innan, för att bygga bakåt │
├──────────────────────────────────────────┼─────────┼─────────────────────────────────┤
│ TheITfiedAB20261019_20260501_20270430.se │ 2026/27 │ –                               │
└──────────────────────────────────────────┴─────────┴─────────────────────────────────┘

Kommer jag ihåg detta nu?

Claude got confused about my 'snapshot' making!

* It started to litter the manual and rules with snapshot-stuff!!
* GOD! It is so much BABY SITTING!
* SURELY ' This vibe-coding way of doing things just Can't go anywhere can it?


## 20261115

Nu vill jag försöka skapa en prouktionsredo Cratychit från tidigare sessioner.

* Vi utgår från koden vi tog fram i session/0a37da4b (../0a37da4b)  
    * Den har vårt nya UI (pyroceph och TUI)
    * Den kod vi redan har i denna session är koden från session/0a37da4b
    * För detaljer se [Överväg att vibe-koda cratchit bokslut på svenska från senaste cratchit-sessioner](../0a37da4b/session.md)
* Vi har en grundstomme till cratchit bokföring i session/60cae9fe (../60cae9fe)
    * Den har funktioner för bokföring och en datamodell vi kan inspireras av
    * För detaljer se [Vibe code a Swedish book keeping app named cratchit - take 3 (Drop feed Claude)](../60cae9fe/session.md)
* Vi har cratchit_ar i session/5b35b2b9 '../5b35b2b9'
    * Den tar en SIE-fil och skapar underlag för en årsredivsining
    * Den är verifierad mot tidigare årsredovisningar samt ett utkast från Årsredovisning Online för årets bokföring ok
    * För detaljer se [# Consider a cratchit app that maps an SIE file to a Swedish Årsredovisning?](../5b35b2b9/session.md)
* Vi har cratchit_ink2 i session/7d4475fb '../7d4475fb'
    * Den tar en SIE-file och genererar ett underlag för inkomstdeklaration 2 (INK2)
    * Den är  verifierad mot tidigare SRU-filer ok
    * Den är nu använda för att generera årets deklaration.
    * För detaljer se [Consider a cratchit app that maps an SIE file to a Swedish SKV Ink_2?](../7d4475fb/session.md)
* Vi har en app walk_pdf i session/9fe36d28 (../9fe36d28)
    * Den producerar ett underlag som visar strukturen för en pdf 
    * Den verkar kapabel att skapa klartext från en pdf (inklusive avkoda en ebook till klartext)
    * [Consider to vibe code a pdf-walker C++ console app?](../9fe36d28/session.md)

I will mix Swedish and English in this markdown depending on what audience I imagine I am writing for.

* In english for Vibe coding and Claude related general experience and observations.
* På svenska för appens faktiska funktioner och relaterat till svensk bokföring.

I have now went 'full steam ahead' and vibe coded what I think is 'from SIE-File' functionality we need for now? 

* A 'don't touch' rule for files in users book keeping folder
* A 'create here' and 'from SIE' and 'From info' that creates in-place (no sub-folder)
* Some enhanced checks and reports on SIE-file to import
* Enhanced meta-data about SIE-files (covered year, status complete/ongoing,inconsistent)

I also had time to adjust tge manual about rebuilding books from scratch using SIE-files

ALl in all 17 steps as recorded by Claude in [status.md](./status.md)
