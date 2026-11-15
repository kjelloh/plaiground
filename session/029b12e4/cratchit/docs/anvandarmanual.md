# Användarmanual för cratchit

cratchit är ett bokföringsprogram för terminalen, på svenska.

Manualen beskriver det som finns i cratchit i dag och växer i takt med programmet. Det som ännu inte finns står under [Kommer senare](#kommer-senare).

## Innehåll

1. [Starta och avsluta](#starta-och-avsluta)
2. [Att använda cratchit](#att-använda-cratchit)
3. [Samlingsmappen](#samlingsmappen)
4. [Bokföringar](#bokföringar)
5. [Dina filer](#dina-filer)
6. [Kommer senare](#kommer-senare)
7. [Ordlista](#ordlista)

## Starta och avsluta

<!-- hjälp: start -->
Du står i början av cratchit. Tryck `0` för att öppna samlingsmappen med dina bokföringar. Har mappen du startade cratchit i ingen bokföring, skapar `1` en ny bokföring i den. Tryck `q` för att avsluta.
<!-- /hjälp -->

Starta cratchit genom att skriva `cratchit` i en terminal och trycka Enter:

```text
Byggd: 14:32:05 2026-11-15 (Release)
Från: /usr/local/bin/cratchit
Version: 0.1.0
Tryck ? för gemensamma val.

  0 Samlingsmapp
Cratchit> 
```

De tre första raderna visar vilket cratchit som körs:

* `Byggd`: när programmet byggdes, och om det är byggt för utveckling (`Debug`) eller för användning (`Release`).
* `Från`: var programfilen ligger.
* `Version`: cratchits version.

Kontrollera raderna när du har byggt eller installerat cratchit. Visar de en gammal tid eller en annan mapp, så kör du ett annat cratchit än du tror.

### Starta i en bokföring

Startar du cratchit i en bokförings mapp, alltså en mapp med en `.cratchit`-mapp som har `project.txt`, öppnar cratchit den bokföringen direkt:

```text
cratchit öppnade bokföringen i arbetsmappen: The ITfied AB
Samlingsmappen kommer ihåg The ITfied AB.
Tryck ? för gemensamma val.

...
Cratchit > Samlingsmapp > The ITfied AB> 
```

* Du står då i bokföringen, som om du hade valt samlingsmappen och sedan bokföringen. Med `-` går du tillbaka till samlingsmappen.
* Samlingsmappen kommer ihåg bokföringen. Nästa gång du startar cratchit i en annan mapp finns den i listan i samlingsmappen.
* cratchit tittar bara i mappen du startar i, inte i mapparna ovanför.
* Två bokföringar med samma `uuid` i `project.txt` räknas som samma bokföring, till exempel en kopia av din bokföring som du provar på. Startar du i kopian kommer samlingsmappen ihåg kopian i stället för originalet, och cratchit säger det: `Samlingsmappen kommer ihåg The ITfied AB här i stället för i …`. Starta i originalet igen för att samlingsmappen ska komma ihåg det.
* Går bokföringen inte att öppna börjar cratchit i början och säger varför.

Tryck `q` för att avsluta.

## Att använda cratchit

### Prompten

Raden som slutar med `>` är prompten. Den visar var du är i programmet, från början till där du står nu:

```text
Cratchit > Samlingsmapp > Lägg till> 
```

Ovanför prompten visar cratchit det som hör till platsen du står på, och sedan valen du har där.

### Välja med en tangent

Varje val har en tangent framför sig. Tryck tangenten, utan Enter, så går du dit:

```text
  1 Befintlig
  2 Ny
Cratchit > Samlingsmapp > Lägg till> 
```

Här tar `1` dig till *Befintlig* och `2` till *Ny*. Menyvalen är korta. Trycker du `?` två gånger förklarar cratchit vad de betyder.

Tryck exakt den tangent som står framför valet. Står det `J Ja` är det versalt `J` som gäller; `j` gör ingenting.

### Hjälp

Tryck `?` var du än står, så visar cratchit de gemensamma valen. Har platsen en hjälptext säger cratchit det, och trycker du `?` en gång till visar cratchit hjälptexten ur den här manualen. Trycker du `?` ännu en gång visas de gemensamma valen igen.

### Gemensamma val

Några tangenter fungerar överallt. De står inte bland valen. Tryck `?` så visar cratchit dem:

| Tangent | Gör |
|---|---|
| `-` | Tillbaka till platsen före |
| `?` | Visa de gemensamma valen. Tryck igen för hjälp om platsen du står på. |
| Enter | Visa valen igen |
| Esc | Töm raden du skriver på (även Ctrl-U) |
| `q` | Avsluta |
| Ctrl-D | Avsluta (även Ctrl-C och Ctrl-Z) |

### Skriva text

Trycker du en tangent som inte är ett val börjar du skriva text. Prompten slutar då med `:` i stället för `>`:

```text
Cratchit > Samlingsmapp > Lägg till: x
```

Medan du skriver är alla tangenter text, även `-`, `?` och `q`.

* **Backspace** suddar det sista tecknet. Suddar du allt väljer du med tangenter igen.
* **Esc** tömmer raden, och sedan väljer du med tangenter igen.
* **Enter** lämnar texten till cratchit.

Tar platsen inte emot text säger cratchit det och låter texten stå kvar, så att du kan sudda den:

```text
Programmet tar inte emot text här.
Cratchit > Samlingsmapp > Lägg till: x
```

### Skriva en sökväg

När cratchit vill ha en mapp står en sökväg redan på raden, och du fortsätter skriva därifrån.

* **Tab** fyller i resten av ett mappnamn. Passar flera mappar fyller cratchit i så långt deras namn är lika. Kan cratchit inte fylla i mer listar den mapparna och skriver raden igen under listan.
* När du letar upp en **SIE-fil** fyller Tab också i filnamn. Listar Tab flera SIE-filer grupperas de på räkenskapsår, med det senaste året först. Inom ett år står den senast skapade filen först, och varje fil har en rad i kolumner:

  ```text
  Cratchit > Ny bokföring här > Från SIE: /Users/anna/Företaget/SIE-filer/
    2026-05-01 – 2027-04-30
      TheITfiedAB20261019_105108.se  pågående       skapad 2026-10-19   17 ver.  t.o.m. 2026-06-30  A 10 · D 3 · M 1
    2025-05-01 – 2026-04-30
      TheITfiedAB20261112_102636.se  fullbordat     skapad 2026-11-12  120 ver.  t.o.m. 2026-04-30  A 59 · D 26 · M 5
      TheITfiedAB20261019_105220.se  ofullbordat    skapad 2026-10-19  116 ver.  t.o.m. 2026-04-30  A 59 · D 26 · M 5
    2015-05-01 – 2016-04-30
      2326 ITFied 1505-1604.se       ofullbordat !  skapad 2016-06-17  247 ver.  t.o.m. 2016-04-30  A 113 · D 63
    Går inte att läsa
      Sie4 fil tom mars -10.se       Unknown encoding: …
    Mappar
      arkiv/
  ```

  Kolumnerna är filens namn, räkenskapsårets status (se nedan), när filen skapades, hur många verifikationer den har, datumet på den senaste och det sista numret i varje serie. En fil utan räkenskapsår står under *Utan räkenskapsår*, och en fil som inte går att läsa under *Går inte att läsa*, med orsaken. Uppgifter som filen saknar lämnas tomma.

  När Tab fyller i en enda fil, eller när hela namnet redan står på raden, visar cratchit filens innehåll på en rad under sökvägen, med räkenskapsåret och dess status först.

#### Räkenskapsårets status i en SIE-fil

cratchit visar vad SIE-filen säger om sitt räkenskapsår, räknat från dagens datum:

* **pågående**: räkenskapsåret har inte tagit slut än.
* **ofullbordat**: räkenskapsåret är slut, men balansräkningen i filen balanserar inte, och inget resultat är bokfört på 8999 (*Årets resultat*). Det gäller till exempel ett år utan bokslut, eller en fil som skapades innan året var slut.
* **fullbordat**: balansräkningen i filen balanserar, det vill säga årets resultat är bokfört.
* **misstämmer**: ett resultat är bokfört på 8999, men balansräkningen balanserar ändå inte.

Ett `!` efter *ofullbordat* betyder att räkenskapsåret är äldre än förra året, alltså slutade för mer än ett år sedan. Ett sådant år ska vara fullbordat. Kontrollera då om det finns en annan fil för samma år.

*Fullbordat* säger bara att bokföringen i filen hänger ihop och att resultatet är bokfört. Det säger inte att årsredovisningen är inlämnad, eller att året är låst i programmet som skapade filen. cratchit hindrar inte att du väljer en fil med någon status; du avgör om filen passar.
* **`-`** går tillbaka så länge du inte har ändrat sökvägen som stod där. Har du ändrat den tömmer du först raden med Esc, och sedan går `-` tillbaka.
* **Enter** lämnar sökvägen till cratchit. Går det inte att använda den visar cratchit varför, och sökvägen står kvar så att du kan rätta den.

### Fel

<!-- hjälp: fel -->
cratchit kunde inte göra det du valde, och orsaken står ovanför prompten. Tryck `-` för att gå tillbaka.
<!-- /hjälp -->

Kan cratchit inte göra det du valde hamnar du alltså på platsen *Fel*.

### Inte än

<!-- hjälp: inte-an -->
Valet du gjorde finns i menyn men cratchit kan inte utföra det än. Vad som saknas står ovanför prompten. Tryck `-` för att gå tillbaka.
<!-- /hjälp -->

Ett val kan finnas i menyn innan cratchit kan utföra det. Det leder då till platsen *Inte än*, som säger vad som saknas, till exempel:

```text
Utvecklaren av cratchit har inte infört att … ännu.
Cratchit > … > Inte än (<valet>)> 
```

Tryck `-` för att gå tillbaka. Just nu gäller det valen i [Årsflöde](#årsflöde) och i dess *Bokslut*.

## Samlingsmappen

<!-- hjälp: samlingsmapp -->
Samlingsmappen listar de bokföringar cratchit kommer ihåg, med namn, organisationsnummer, när de skapades och mappen de ligger i. Tryck en bokförings siffra för att öppna den. Tryck `+` för att lägga till en bokföring.
<!-- /hjälp -->

Tryck `0` i början för att öppna samlingsmappen.

Samlingsmappen listar de bokföringar den kommer ihåg, med tangenterna `1` till `9`, i bokstavsordning efter namn:

```text
/Users/anna/Documents/cratchit
    Namn           Org.nr       Skapad      Mapp
  1 Borttagen AB                            /Users/anna/Borttagen AB  (finns inte längre)
  2 Exempel AB     556677-8899  2026-11-09  Exempel AB
  3 The ITfied AB  556782-8172  2026-10-23  ~/Documents/Bokföringar/The ITfied AB
  + Lägg till bokföring
Cratchit > Samlingsmapp> 
```

* *Mapp* visar var bokföringen ligger. Ligger den i samlingsmappen står bara mappens namn, och ligger den i din hemkatalog börjar sökvägen med `~`.
* Uppgifterna hämtas från bokföringen själv varje gång listan visas. Samlingsmappen kommer bara ihåg var bokföringarna ligger.
* *(finns inte längre)* betyder att mappen är borttagen eller flyttad. Har du flyttat bokföringen lägger du till den igen på det nya stället (se [Flytta en bokföring](#flytta-en-bokföring)).
* *(går inte att öppna)* betyder att mappen finns, men att den inte innehåller den bokföring samlingsmappen kommer ihåg.

### Glömma en bokföring

<!-- hjälp: glom -->
Bokföringen finns inte längre där samlingsmappen tror, eller går inte att öppna. Orsaken står ovanför prompten. Tryck `0` för att samlingsmappen ska glömma den, eller `-` för att gå tillbaka utan att glömma. Inga filer tas bort.
<!-- /hjälp -->

Väljer du en bokföring som är märkt *(finns inte längre)* eller *(går inte att öppna)* visar cratchit varför den inte går att öppna och erbjuder `0` *Glöm*:

```text
Bokföringen fanns i /Users/anna/Borttagen AB, men mappen finns inte längre.
Den kan vara borttagen eller flyttad. Har du flyttat den kan du lägga till den på det nya stället med + i samlingsmappen.
  0 Glöm
Cratchit > Samlingsmapp > Borttagen AB> 
```

Tryck `0`, så glömmer samlingsmappen bokföringen och du kommer tillbaka till samlingsmappen, där den inte längre finns i listan. Glöm tar bara bort bokföringen ur samlingsmappens lista. Inga filer eller mappar tas bort.

Har du fler än nio bokföringar visar cratchit de nio första och säger att det finns fler.

## Bokföringar

En bokföring är bokföringen för en juridisk person, till exempel ett aktiebolag. Den ligger i en egen mapp, som kan ligga var som helst på datorn.

### Öppna en bokföring

<!-- hjälp: bokforing -->
Du har öppnat en bokföring. Ovanför prompten står mappen den ligger i. Valen är bokföringens räkenskapsår: `0` är det år som dagens datum ligger i, och `1`, `2`, `3` är åren bakåt. Efter varje år står hur mycket som är bokfört. Tryck ett års siffra för att öppna året. Tryck `-` för att gå tillbaka.
<!-- /hjälp -->

Tryck bokföringens siffra i samlingsmappen. Prompten får då bokföringens namn, och cratchit visar mappen den ligger i:

```text
/Users/anna/Documents/Bokföringar/The ITfied AB
  0 2026-05-01 – 2027-04-30  bokfört: 37 verifikationer
  1 2025-05-01 – 2026-04-30  bokfört: 116 verifikationer
  2 2024-05-01 – 2025-04-30  bokfört: 115 verifikationer
  3 2023-05-01 – 2024-04-30  bokfört: inget
Cratchit > Samlingsmapp > The ITfied AB> 
```

Valen är bokföringens räkenskapsår. `0` är det aktuella året, det som dagens datum ligger i, och `1`, `2`, `3` är åren före det, på samma sätt som i en SIE-fil men utan minustecken. Listan går tillbaka till det äldsta året med bokföring, och sist står året före det, med `bokfört: inget`. Det året finns där för att du ska kunna importera ett tidigare år från en SIE-fil. När du har gjort det står nästa tidigare år sist, och så vidare. Listan visar varje års första och sista dag. När du öppnar ett år får det ett kort namn i prompten: `2025` för ett år som följer kalenderåret, och till exempel `2025/26` för ett som går över två kalenderår. Har bokföringen fler än tio räkenskapsår visar cratchit de tio senaste och säger att det finns fler.

Räkenskapsåren finns oavsett om något är bokfört i dem. När ett nytt räkenskapsår börjar dyker det upp som `0` med `bokfört: inget`, och de andra åren flyttas ett steg. Du behöver inte skapa det. Varje år efter det första är tolv månader och börjar dagen efter förra årets sista dag. Ett första år som är kortare eller längre följs alltså av helt vanliga år. Ett räkenskapsår som läggs om kan du bara ta in från en SIE-fil (se [Importera från SIE](#importera-från-sie)).

```text
  0 2027-05-01 – 2028-04-30  bokfört: inget
  1 2026-05-01 – 2027-04-30  bokfört: 37 verifikationer
```

Ett år där inget är bokfört fungerar som de andra: du kan importera till det och exportera det.

### Räkenskapsår

<!-- hjälp: rakenskapsar -->
Du har öppnat ett räkenskapsår i bokföringen. Överst står vilken period det gäller och vad året innehåller. Under det visar cratchit årets verifikationer och utkast, de nyaste först. Listan är bara en vy, raderna är inga val. Tryck `0` för årets arbete i *Årsflöde*, `1` för att exportera året till en SIE-fil, eller `2` för att importera en SIE-fil för året. Tryck `-` för att gå tillbaka till bokföringen.
<!-- /hjälp -->

Tryck räkenskapsårets siffra i bokföringen. Prompten får då årets namn:

```text
Räkenskapsår 2025-05-01 – 2026-04-30
Hela året: 116 verifikationer
  M 5         2026-04-30         89.98  Momsrapport_2026-04-01-2026-04-30
  A 59        2026-04-22       2020.00  Skatt
  E 25        2026-04-20        361.25  LOOPIA AB
  …
  E 21        2026-03-30       1233.75  LOOPIA AB
  … 106 äldre
  0 Årsflöde
  1 Exportera till SIE
  2 Importera från SIE
Cratchit > Samlingsmapp > The ITfied AB > 2025/26> 
```

Andra raden säger vad listan gäller och vad den innehåller, till exempel `Hela året: 37 verifikationer, 1 utkast`. Är inget bokfört står det `Hela året: inget bokfört`. Än så länge visar listan alltid hela året.

Listan visar de tio nyaste raderna. Finns det fler står det sist hur många äldre som inte visas. En verifikation visar serie och nummer, datum, belopp och text. Ett utkast visar i stället hur långt det har kommit:

| Står det | Betyder |
|---|---|
| `händelse` | Utkastet har datum och kanske text och belopp, men inga konteringar |
| `kontoutdrag` | Utkastet har konteringar, till exempel från ett kontoutdrag, men ingen text |
| `obalanserat` | Utkastet har text och konteringar, men konteringarna balanserar inte eller stämmer inte med beloppet |
| `klart` | Utkastet kan bokföras |

Du kan ännu inte skapa utkast i cratchit.

### Årsflöde

<!-- hjälp: arsflode -->
Årsflöde samlar arbetet med räkenskapsåret, i den ordning det brukar göras under året och efter det. Tryck valets siffra. Bara `3 Bokslut` leder vidare än så länge, de andra valen säger vad som saknas. Tryck `-` för att gå tillbaka till räkenskapsåret.
<!-- /hjälp -->

Tryck `0` i räkenskapsåret:

```text
Arbetet med räkenskapsåret, i den ordning det brukar göras.
  0 Utkast
  1 Bokfört
  2 Momsrapport
  3 Bokslut
  4 Årsredovisning
  5 Årsstämma
  6 Rapport till Bolagsverket
  7 Deklaration till SKV
Cratchit > Samlingsmapp > The ITfied AB > 2025/26 > Årsflöde> 
```

Valen finns i menyn, men cratchit kan inte utföra dem än (se [Inte än](#inte-än) och [Kommer senare](#kommer-senare)).

#### Bokslut

<!-- hjälp: bokslut -->
Bokslutet avslutar räkenskapsåret. Tryck `0` för avstämning mot kontoutdrag. Den finns inte än. Tryck `-` för att gå tillbaka till Årsflöde.
<!-- /hjälp -->

### Importera från SIE

<!-- hjälp: importera-sie -->
Skriv sökvägen till en SIE-fil för det här räkenskapsåret och tryck Enter. Tab fyller i mappar och SIE-filer. cratchit jämför filen med bokföringen och visar om importen går att göra, innan något importeras. Så länge du inte har ändrat sökvägen går `-` tillbaka.
<!-- /hjälp -->

I ett räkenskapsår tar du in en SIE-fil för samma år från ett annat bokföringsprogram, till exempel nya verifikationer. Filen måste gälla just det räkenskapsåret. Gäller den ett annat år säger cratchit det och importerar ingenting.

1. Tryck `2` (*Importera från SIE*) i räkenskapsåret.
2. Skriv sökvägen till SIE-filen. Sökvägen börjar i bokföringens mapp, och Tab fyller i mappar och SIE-filer.
3. Tryck Enter. cratchit jämför filen med bokföringen, serie för serie, och säger om importen går att göra.
4. Kontrollera det som står. Tryck `J` (*Ja*) för att importera, eller `N` (*Nej*) för att välja en annan fil.

```text
SIE-fil: /Users/anna/Företaget/SIE-filer/TheITfiedAB20261112_102636.se
  Räkenskapsår:   2025-05-01 – 2026-04-30, fullbordat
  Skapad:         2026-11-12 (Fortnox 3.62.17)
  Verifikationer: 120 t.o.m. 2026-04-30
Jämfört med bokföringen (2025/26):
  A  lika, t.o.m. A 59
  D  lika, t.o.m. D 26
  I  bokföringen t.o.m. I 1, filen t.o.m. I 5, nya: I 2–5
  Konton: 0 nya, 0 med ändrat namn eller ändrade uppgifter
Importen går att göra: 4 nya verifikationer.
Importera SIE-filen?
  J Ja
  N Nej
Cratchit > Samlingsmapp > The ITfied AB > 2025/26 > Importera från SIE > Bekräfta> 
```

Går importen inte att göra står det varför, och verifikationerna i bokföringen som filen skulle ta bort eller byta ut står under sin serie. Då finns bara `N` (*Tillbaka*):

```text
Jämfört med bokföringen (2025/26):
  D  bokföringen t.o.m. D 28, filen t.o.m. D 26
      D 27  2026-04-12  Loopia Faktura  311.25  finns inte i filen
      D 28  2026-04-30  Telia  1250.00  finns inte i filen
Importen går inte att göra:
  Bokföringen har 2 verifikationer som filen skulle ta bort eller byta ut. …
  N Tillbaka
```

<!-- hjälp: bekrafta-import -->
Ovanför prompten står SIE-filen och hur den förhåller sig till bokföringen, serie för serie: hur långt bokföringen och filen räcker, och vilka verifikationer som är nya. Sist står om importen går att göra, och annars varför. Ändrade verifikationer listas tio i taget, och `2` (*Mer…*) visar fler. Tryck `J` för att importera. Tryck `N` eller `-` för att gå tillbaka och välja en annan fil; sökvägen börjar då i mappen där du valde filen.
<!-- /hjälp -->

När importen är klar går cratchit tillbaka till räkenskapsåret, som säger vad som importerades. SIE-filen ligger kvar där den låg (se [Dina filer](#dina-filer)).

cratchit säger direkt under prompten att filen inte går att använda, och visar ingen jämförelse, om:

* filen inte är en SIE-fil, har fel eller saknar räkenskapsår,
* filen gäller ett annat räkenskapsår än det du står i (se nedan om år som ansluter),
* filens räkenskapsår är nytt men inte ansluter till bokföringen. Ett nytt år måste börja dagen efter det senaste bokförda årets sista dag, eller sluta dagen före det äldsta årets första dag. Bokföringen får inga luckor mellan åren.

*Bekräfta* visar jämförelsen, men säger att importen inte går att göra, om:

* filen gäller ett annat organisationsnummer än bokföringen,
* en serie i filen har en lucka, till exempel `Serie A i filen har en lucka: A 12 saknas.` Filen ska ansluta till bokföringen utan luckor,
* allt i filen redan finns i bokföringen,
* importen skulle ta bort eller byta ut verifikationer som redan finns i bokföringen,
* en verifikation som redan finns har ett annat datum eller en annan text i filen.

**Ett år med andra datum.** I ett år där inget är bokfört tar cratchit emot en SIE-fil för ett år med andra datum, om filens år ansluter till bokföringen och är högst 18 månader:

* i det första året efter det senaste bokförda året, om filens år börjar dagen efter det årets sista dag. Så tar du in en omläggning av räkenskapsåret.
* i året före det äldsta bokförda året, om filens år slutar dagen före det årets första dag. Så tar du in ett första år som är kortare eller längre än tolv månader.

*Bekräfta* visar då vilket år cratchit räknade med:

```text
  Räkenskapsår:   2026-01-01 – 2026-04-30, ofullbordat (cratchit räknade med 2026-01-01 – 2026-12-31)
  Ansvar:         cratchit kontrollerar inte Skatteverkets godkännande av en omläggning. Du ansvarar för att räkenskapsåret är korrekt.
```

Efter importen har året filens datum, och de följande åren räknas från det, tolv månader i taget. I alla andra år måste filen ha exakt samma datum som året.

**Ändrade verifikationer.** En verifikation som redan är bokförd får enligt SIE bara ändras genom att rader stryks (`#BTRANS`) och nya rader läggs till (`#RTRANS`). Det som redan står får inte ändras. Sådana ändringar tar cratchit emot. *Bekräfta* listar då varje ändrad verifikation, till exempel `A 12  2025-06-03  Loopia: 1 rad struken, 1 rad tillagd`, tio i taget, och `2` (*Mer…*) visar fler. Ändringarna skrivs också i loggen.

### Exportera till SIE

<!-- hjälp: exportera-sie -->
Tryck `1` *Allt* för att skriva hela räkenskapsåret, med balanser och alla verifikationer, till en SIE-fil. Tryck `2` *Skillnad* för att jämföra året med en SIE-fil från ett annat program och skriva bara de verifikationer som filen saknar. Filen hamnar i bokföringens mapp.
<!-- /hjälp -->

Med en SIE-fil kan du lämna över ett räkenskapsår till ett annat bokföringsprogram. Tryck `1` (*Exportera till SIE*) i räkenskapsåret. Där finns två val:

* `1` *Allt*: hela räkenskapsåret, med kontoplan, ingående och utgående balanser, resultat och alla verifikationer. Filen heter till exempel `sie_2025-26_allt.se`.
* `2` *Skillnad*: bara de verifikationer som finns i räkenskapsåret men inte i en SIE-fil som du väljer, utan balanser. Filen heter till exempel `sie_2025-26_skillnad.se`.

Filen hamnar i bokföringens mapp, och cratchit skriver aldrig över en tidigare export: finns filen redan får den nya filen ett nummer, till exempel `sie_2025-26_allt_2.se`. När filen är skriven går cratchit tillbaka till räkenskapsåret och säger vad som skrevs:

```text
Räkenskapsår 2025-05-01 – 2026-04-30
cratchit skrev sie_2025-26_allt.se i /Users/anna/Documents/cratchit/The ITfied AB: 115 verifikationer och balanser.
  1 Exportera till SIE
Cratchit > Samlingsmapp > The ITfied AB > 2025/26> 
```

<!-- hjälp: skillnad-sie -->
Skriv sökvägen till SIE-filen att jämföra med och tryck Enter. Filen ska vara en export som du just har gjort från programmet du ska föra över till, till exempel Fortnox, så att den visar hur bokföringen ser ut där nu. Är filen äldre kan verifikationer som redan finns i programmet komma med igen, och då läggs de till en gång till. Sökvägen börjar i bokföringens mapp, och Tab fyller i mappar och SIE-filer.
<!-- /hjälp -->

*Skillnad* är till för när du bokför i cratchit och vill föra över det nya till ett annat program. Exportera först året från det andra programmet till en SIE-fil, och välj sedan den filen här. cratchit jämför verifikationerna efter datum och konton och belopp, inte efter nummer.

**Jämför alltid med en färsk export.** Skillnaden räknas mot SIE-filen du väljer, inte mot programmet självt. Den blir därför bara rätt om filen visar hur bokföringen i programmet ser ut just nu. Gör så här varje gång:

1. Exportera räkenskapsåret från programmet du ska föra över till.
2. Välj den filen i *Skillnad* och skriv skillnaden.
3. Läs in skillnadsfilen i programmet.

Nästa gång börjar du om från steg 1. Den gamla exporten saknar ju det du just läste in, så en jämförelse med den skulle skriva samma verifikationer igen. Är filen inte skapad i dag, eller saknar den datum, frågar cratchit i jämförelsen om det är den senaste exporten:

```text
Jämfört med TheITfiedAB.se (Fortnox 3.0, skapad 2026-10-19):
Filen skapades 2026-10-19. Är det den senaste exporten från programmet?
  I räkenskapsåret, inte i filen (2):
    A 116   2026-04-28       1250.00  Loopia
    A 117   2026-04-30        310.00  Bankavgift
  I filen, inte i räkenskapsåret (0):
  1 Skriv skillnaden till en SIE-fil
Cratchit > Samlingsmapp > The ITfied AB > 2025/26 > Exportera till SIE > Skillnad > TheITfiedAB.se> 
```

<!-- hjälp: jamforelse-sie -->
Ovanför prompten står vilka verifikationer som bara finns i räkenskapsåret och vilka som bara finns i SIE-filen. Tryck `1` för att skriva de verifikationer som bara finns i räkenskapsåret till en SIE-fil. Har filen verifikationer som inte finns i räkenskapsåret, eller som ser annorlunda ut där, skriver cratchit ingen fil. Tryck `-` för att gå tillbaka.
<!-- /hjälp -->

Ska du föra över till Fortnox eller ett liknande program, använd *Skillnad* och inte *Allt*. Program som Fortnox lägger till alla verifikationer i filen, med nya nummer i samma serie, men kan inte ändra eller ta bort de verifikationer som redan finns. En fil med hela året skulle därför dubblera verifikationerna i programmet. Exportera först året från programmet, och jämför med den filen.

Har SIE-filen verifikationer som inte finns i räkenskapsåret, eller som ser annorlunda ut där, skriver cratchit ingen fil. Program som Fortnox kan bara lägga till verifikationer, så de måste stämma först: gör ändringen i programmet, eller ta in den i cratchit. Finns alla verifikationer redan i filen säger cratchit att det inte finns något att exportera.

### Lägg till

<!-- hjälp: lagg-till -->
Tryck `1` *Befintlig* för att lägga till en bokföring som redan finns men inte står i samlingsmappens lista, till exempel en som du har flyttat, kopierat eller tagit med från en annan dator. Tryck `2` *Ny* för att skapa en ny bokföring, från en SIE-fil eller från uppgifter om företaget.
<!-- /hjälp -->

Du kommer hit med `+` i samlingsmappen.

### Lägga till en befintlig bokföring

<!-- hjälp: oppna-befintlig -->
Skriv sökvägen till mappen som bokföringen ligger i och tryck Enter. Tab fyller i mappnamn. cratchit lägger till bokföringen i samlingsmappens lista och går tillbaka dit. Så länge du inte har ändrat sökvägen går `-` tillbaka. Annars tömmer Esc raden först.
<!-- /hjälp -->

Har du en bokföring som samlingsmappen inte listar tar du in den så här:

1. Tryck `+` (*Lägg till bokföring*) i samlingsmappen.
2. Tryck `1` (*Befintlig*).
3. Skriv sökvägen till mappen som bokföringen ligger i. Sökvägen börjar i samlingsmappen, och Tab fyller i mappnamnen (se [Skriva en sökväg](#skriva-en-sökväg)).
4. Tryck Enter.

```text
Cratchit > Samlingsmapp > Lägg till > Befintlig: /Users/anna/Documents/Bokföringar/The ITfied AB/
Samlingsmappen kommer ihåg The ITfied AB.
/Users/anna/Documents/cratchit
    Namn           Org.nr       Skapad      Mapp
  1 The ITfied AB  556782-8172  2026-10-23  ~/Documents/Bokföringar/The ITfied AB
  + Lägg till bokföring
Cratchit > Samlingsmapp> 
```

cratchit går tillbaka till samlingsmappen, där bokföringen nu finns i listan. Där öppnar du den med dess siffra, som alla andra bokföringar.

Ligger det ingen bokföring i mappen säger cratchit det, och sökvägen står kvar så att du kan rätta den.

### Flytta en bokföring

Allt som hör till en bokföring ligger i dess mapp. Flytta eller kopiera mappen dit du vill ha den, och lägg sedan till den på det nya stället med `+` och `1` (*Befintlig*). cratchit säger då "Samlingsmappen kommer ihåg … på det nya stället.", och listan visar den nya platsen.

### Ny bokföring

<!-- hjälp: ny-bokforing -->
Välj hur den nya bokföringen ska skapas: `1` från en SIE-fil eller `2` från namn, organisationsnummer och räkenskapsår.
<!-- /hjälp -->

Tryck `+` och sedan `2` (*Ny*). Där finns två val:

* `1` *Från SIE*: skapa bokföringen från en SIE-fil (se nedan).
* `2` *Från uppgifter*: skapa bokföringen från namn, organisationsnummer och räkenskapsår (se [Ny bokföring från uppgifter](#ny-bokföring-från-uppgifter)).

### Ny bokföring från en SIE-fil

<!-- hjälp: fran-sie -->
Skriv sökvägen till SIE-filen och tryck Enter. Tab fyller i mappar och SIE-filer. cratchit läser filen och visar vad den innehåller innan något skapas. Så länge du inte har ändrat sökvägen går `-` tillbaka.
<!-- /hjälp -->

En SIE-fil är en fil med bokföring som du har hämtat från ett annat bokföringsprogram. Så skapar du en bokföring från den:

1. Tryck `+`, `2` (*Ny*) och `1` (*Från SIE*).
2. Skriv sökvägen till SIE-filen. Sökvägen börjar i samlingsmappen, och Tab fyller i mappar och SIE-filer.
3. Tryck Enter. cratchit läser filen och visar vad den innehåller.
4. Tryck `J` (*Ja*) för att skapa bokföringen, eller `N` (*Nej*) för att välja en annan fil.

```text
SIE-fil: /Users/anna/Documents/cratchit/TheITfiedAB.se
  Företag:        The ITfied AB (556782-8172)
  Räkenskapsår:   2026-05-01 – 2027-04-30, pågående
  Skapad:         2026-10-19 (Fortnox 3.62.17)
  Verifikationer: 42 t.o.m. 2026-10-12  A 21 · D 11 · M 10
  Konton:         501
Skapa bokföringen från SIE-filen?
  J Ja
  N Nej
Cratchit > Samlingsmapp > Lägg till > Ny > Från SIE > Bekräfta> 
```

<!-- hjälp: bekrafta-sie -->
Ovanför prompten står vad cratchit hittade i SIE-filen: företaget, räkenskapsåret och dess status (pågående, ofullbordat, fullbordat eller misstämmer), när och med vilket program filen skapades, verifikationerna och kontona. En rad *Varning* säger till om ett äldre år är ofullbordat eller om filen misstämmer. Tryck `J` för att skapa bokföringen. Tryck `N` eller `-` för att gå tillbaka och välja en annan fil; sökvägen börjar då i mappen där du valde filen.
<!-- /hjälp -->

cratchit skapar en ny mapp med företagets namn bredvid SIE-filen och tar in hela filen. SIE-filen ligger kvar där den låg (se [Dina filer](#dina-filer)). Sedan går cratchit tillbaka till samlingsmappen, där den nya bokföringen nu finns i listan. (Med [Ny bokföring här](#ny-bokföring-här) hamnar bokföringen i stället i mappen du startade i.)

cratchit skapar ingenting om filen inte går att använda, till exempel om den inte är en SIE-fil, har fel eller saknar företagets namn eller räkenskapsår. Det gäller också om det redan finns en mapp med företagets namn bredvid filen. Orsaken står då under prompten, och sökvägen står kvar så att du kan rätta den.

### Ny bokföring från uppgifter

<!-- hjälp: fran-uppgifter -->
Välj en uppgift med dess siffra och fyll i den. När namn, organisationsnummer och räkenskapsår är ifyllda skapar Enter bokföringen. Du kan ändra en uppgift så många gånger du vill innan dess.
<!-- /hjälp -->

Har du ingen SIE-fil skapar du bokföringen från uppgifter om företaget. Tryck `+`, `2` (*Ny*) och `2` (*Från uppgifter*). Där fyller du i ett formulär:

```text
  1 Namn: <ange organisationens namn>
  2 Organisationsnummer: <ange organisationens organisationsnummer>
  3 Räkenskapsår: <ange räkenskapsårets första månad>
  4 Första årets längd: 12 månader
Cratchit > Samlingsmapp > Lägg till > Ny > Från uppgifter> 
```

1. Tryck en uppgifts siffra. Raden är tom om uppgiften inte är ifylld, och visar det du skrev om den är det.
2. Skriv uppgiften och tryck Enter. Går den att använda kommer du tillbaka till formuläret, där den nu står. Går den inte att använda visar cratchit varför, och texten står kvar så att du kan rätta den.
3. Vill du inte ändra något trycker du `-` innan du har ändrat raden. Har du börjat ändra tömmer Esc raden först. Uppgiften är då som den var.
4. När namn, organisationsnummer och räkenskapsår är ifyllda trycker du Enter i formuläret. cratchit skapar då bokföringen och går tillbaka till samlingsmappen, där den finns i listan.

```text
Tryck Enter för att skapa bokföringen i /Users/anna/Documents/cratchit/Exempel AB.
Bokföringen får ingen kontoplan än.
  1 Namn: Exempel AB
  2 Organisationsnummer: 556677-8899
  3 Räkenskapsår: 2026-05-01 – 2027-04-30
  4 Första årets längd: 12 månader
Cratchit > Samlingsmapp > Lägg till > Ny > Från uppgifter> 
```

Bokföringen får en mapp med företagets namn i samlingsmappen. Den får än så länge ingen kontoplan (se [Kommer senare](#kommer-senare)).

<!-- hjälp: uppgifter-namn -->
Skriv organisationens namn och tryck Enter. Bokföringen får en mapp med det namnet i samlingsmappen, så det får inte redan finnas en mapp med samma namn där.
<!-- /hjälp -->

<!-- hjälp: uppgifter-orgnr -->
Skriv organisationsnumret och tryck Enter, med eller utan bindestreck, till exempel 556677-8899. cratchit kontrollerar att kontrollsiffran stämmer.
<!-- /hjälp -->

<!-- hjälp: uppgifter-rakenskapsar -->
Skriv räkenskapsårets första månad och tryck Enter, till exempel 2026-05. Räkenskapsåret börjar den första dagen i den månaden.
<!-- /hjälp -->

<!-- hjälp: uppgifter-langd -->
Skriv hur många månader det första räkenskapsåret är och tryck Enter, 1 till 18. Oftast är det 12, men ett nystartat företags första räkenskapsår kan vara kortare eller längre, högst 18 månader.
<!-- /hjälp -->

Ett nystartat företag kan ha ett första räkenskapsår som är kortare eller längre än tolv månader. Startar företaget till exempel i maj och ska ha kalenderår, blir det första räkenskapsåret 8 månader: maj till december.

### Ny bokföring här

<!-- hjälp: ny-bokforing-har -->
Bokföringen skapas i mappen du startade cratchit i. Välj `1` för att skapa den från en SIE-fil, eller `2` från namn, organisationsnummer och räkenskapsår. Andra filer i mappen rör cratchit inte.
<!-- /hjälp -->

Vill du ha bokföringen i en mapp som redan finns, till exempel företagets mapp med avtal och fakturor, startar du cratchit i den mappen:

```sh
cd ~/Företaget
cratchit
```

Har mappen ingen bokföring finns valet `1 Ny bokföring här`. Det har samma två val som [Ny bokföring](#ny-bokföring) i samlingsmappen:

```text
Bokföringen hamnar i /Users/anna/Företaget.
  1 Från SIE
  2 Från uppgifter
Cratchit > Ny bokföring här> 
```

* `1 Från SIE` fungerar som [Ny bokföring från en SIE-fil](#ny-bokföring-från-en-sie-fil), men sökvägen börjar i mappen du startade i. Leta upp SIE-filen med Tab. SIE-filen ligger kvar där den låg.
* `2 Från uppgifter` är formuläret i [Ny bokföring från uppgifter](#ny-bokföring-från-uppgifter). Namnet är bara organisationens namn, inget mappnamn.

Skillnaden mot samlingsmappens *Ny* är var bokföringen hamnar: i mappen du startade i, i en `.cratchit`-mapp, och inte i en ny mapp. När bokföringen är skapad hamnar du i samlingsmappen, som kommer ihåg bokföringen och listar den:

```text
cratchit skapade bokföringen The ITfied AB.
/Users/anna/Documents/cratchit
    Namn           Org.nr       Skapad      Mapp
  1 The ITfied AB  556782-8172  2026-11-15  ~/Företaget
  + Lägg till bokföring
Cratchit > Samlingsmapp> 
```

Går du tillbaka utan att skapa något kommer du till början som vanligt. Nästa gång du startar cratchit i mappen öppnas bokföringen direkt (se [Starta i en bokföring](#starta-i-en-bokföring)).

cratchit skapar ingen bokföring i en mapp som redan har en `.cratchit`-mapp, i din hemkatalog, i samlingsmappen eller i en mapp som ligger inuti en annan bokföring. Då leder `1` till *Fel*, som säger varför.

<!-- hjälp: har-namn -->
Skriv organisationens namn och tryck Enter, till exempel The ITfied AB. Namnet är bara namnet; bokföringen hamnar i mappen du startade cratchit i.
<!-- /hjälp -->

## Dina filer

cratchit verkar utan att märkas. cratchit flyttar, döper om, ändrar och tar aldrig bort dina filer. cratchit skriver i sina egna `.cratchit`-mappar, och skapar synliga filer och mappar bara när du ber om just det, till exempel en ny bokföring i en ny mapp eller en export till SIE.

När du skapar en bokföring från en SIE-fil eller importerar en, ligger filen kvar där den låg. cratchit för i stället en *förteckning* över de filer bokföringen har fått något från: vad filen hette, var den låg, när den lästes in och vad den gav. Förteckningen känner igen en fil på dess innehåll, så den vet vilken fil det var även om du senare flyttar filen eller byter namn på den.

Förteckningen ligger i bokföringens `.cratchit`-mapp, en för hela bokföringen och en för varje räkenskapsår. cratchit visar den inte än.

Har en bokföring en mapp `importerade_sie_filer` från en tidigare version av cratchit, rör cratchit inte den. Du kan flytta tillbaka filerna eller ta bort mappen när du vill.

## Kommer senare

* Valen i Årsflöde: utkast som du skapar, ändrar och bokför, arbete med årets verifikationer, momsrapport, årsredovisning, årsstämma, rapport till Bolagsverket och deklaration till Skatteverket.
* Avstämning mot kontoutdrag i bokslutet, till exempel 1920 mot plusgirot, 1930 mot sparkontot och 1630 mot skattekontot.

* En kontoplan för en ny bokföring från uppgifter. cratchit kommer att erbjuda BAS-kontoplanen.
* Import av en SIE-fil som tar bort eller byter ut verifikationer som redan finns i bokföringen, eller ändrar deras datum eller text.

## Ordlista

| Ord | Betyder |
|---|---|
| bokföring | Bokföringen för en juridisk person, i en egen mapp |
| samlingsmapp | Där cratchit håller reda på dina bokföringar |
| sökväg | Var en mapp ligger på datorn, till exempel `/Users/anna/Documents` |
| prompt | Raden där cratchit väntar på din tangent eller text |
| organisationsnummer | Företagets nummer hos Bolagsverket, tio siffror, till exempel 556677-8899 |
| kontoplan | Listan över bokföringens konton, till exempel BAS-kontoplanen |
| SIE-fil | En fil med bokföring i ett standardformat, som bokföringsprogram använder för att lämna över bokföring till varandra |
| räkenskapsår | Den period, oftast tolv månader, som bokföringen räknas och avslutas för |
| verifikation | En bokförd affärshändelse, med serie och nummer, till exempel `A 12` |
| utkast | En affärshändelse som ännu inte är bokförd. Allt i den kan ändras |
