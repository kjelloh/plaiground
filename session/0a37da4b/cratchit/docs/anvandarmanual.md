# Användarmanual för cratchit

cratchit är ett bokföringsprogram för terminalen, på svenska.

Manualen beskriver det som finns i cratchit i dag och växer i takt med programmet. Det som ännu inte finns står under [Kommer senare](#kommer-senare).

## Innehåll

1. [Starta och avsluta](#starta-och-avsluta)
2. [Att använda cratchit](#att-använda-cratchit)
3. [Samlingsmappen](#samlingsmappen)
4. [Bokföringar](#bokföringar)
5. [Kommer senare](#kommer-senare)
6. [Ordlista](#ordlista)

## Starta och avsluta

Skriv `cratchit` i en terminal och tryck Enter:

```text
Tryck ? för gemensamma val.

  0 Samlingsmapp
Cratchit> 
```

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
  1 Öppna befintlig
  2 Ny bokföring
Cratchit > Samlingsmapp > Lägg till> 
```

Här tar `1` dig till *Öppna befintlig* och `2` till *Ny bokföring*.

### Gemensamma val

Några tangenter fungerar överallt. De står inte bland valen. Tryck `?` så visar cratchit dem:

| Tangent | Gör |
|---|---|
| `-` | Tillbaka till platsen före |
| `?` | Visa de gemensamma valen |
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
* **`-`** går tillbaka så länge du inte har ändrat sökvägen som stod där. Har du ändrat den tömmer du först raden med Esc, och sedan går `-` tillbaka.
* **Enter** lämnar sökvägen till cratchit. Går det inte att använda den visar cratchit varför, och sökvägen står kvar så att du kan rätta den.

### Fel

Kan cratchit inte göra det du valde hamnar du på platsen *Fel*, som visar orsaken. Tryck `-` för att gå tillbaka.

### Inte än

Några val finns i menyerna innan cratchit kan utföra dem. De leder till platsen *Inte än*, som säger vad som saknas:

```text
Utvecklaren av cratchit har inte infört att skapa en ny bokföring från en SIE-fil ännu.
Cratchit > Samlingsmapp > Lägg till > Ny bokföring > Inte än (Från SIE)> 
```

Tryck `-` för att gå tillbaka.

## Samlingsmappen

Samlingsmappen håller reda på dina bokföringar. Tryck `0` i början för att öppna den.

Samlingsmappen listar de bokföringar den kommer ihåg, med tangenterna `1` till `9`, i bokstavsordning efter namn:

```text
/Users/anna/Documents/cratchit
  1 The ITfied AB
  + Lägg till bokföring
Cratchit > Samlingsmapp> 
```

Har du fler än nio bokföringar visar cratchit de nio första och säger att det finns fler.

## Bokföringar

En bokföring är bokföringen för en juridisk person, till exempel ett aktiebolag. Den ligger i en egen mapp, som kan ligga var som helst på datorn.

### Öppna en bokföring

Tryck bokföringens siffra i samlingsmappen. Prompten får då bokföringens namn, och cratchit visar mappen den ligger i:

```text
/Users/anna/Documents/Bokföringar/The ITfied AB
Cratchit > Samlingsmapp > The ITfied AB> 
```

Än så länge finns det inga val i en öppnad bokföring.

### Lägga till en befintlig bokföring

Har du en bokföring som samlingsmappen inte listar tar du in den så här:

1. Tryck `+` (*Lägg till bokföring*) i samlingsmappen.
2. Tryck `1` (*Öppna befintlig*).
3. Skriv sökvägen till mappen som bokföringen ligger i. Tab fyller i mappnamnen (se [Skriva en sökväg](#skriva-en-sökväg)).
4. Tryck Enter.

```text
Cratchit > Samlingsmapp > Lägg till > Öppna befintlig: /Users/anna/Documents/Bokföringar/The ITfied AB/
Samlingsmappen kommer ihåg The ITfied AB.
/Users/anna/Documents/Bokföringar/The ITfied AB
Cratchit > Samlingsmapp > Lägg till > Öppna befintlig > The ITfied AB> 
```

cratchit öppnar bokföringen, och samlingsmappen kommer ihåg den. När du går tillbaka till samlingsmappen finns bokföringen i listan.

Ligger det ingen bokföring i mappen säger cratchit det, och sökvägen står kvar så att du kan rätta den.

### Flytta en bokföring

Allt som hör till en bokföring ligger i dess mapp. Flytta eller kopiera mappen dit du vill ha den, och lägg sedan till den på det nya stället med *Öppna befintlig*. Samlingsmappen kommer då ihåg det nya stället i stället för det gamla.

### Ny bokföring

Tryck `+` och sedan `2` (*Ny bokföring*). Där finns två val:

* `1` *Från SIE*: skapa bokföringen från en SIE-fil.
* `2` *Från uppgifter*: skapa bokföringen från namn, organisationsnummer och räkenskapsår.

Inget av dem finns än. Båda leder till [Inte än](#inte-än).

## Kommer senare

Det här finns som val men leder till [Inte än](#inte-än):

* Skapa en ny bokföring från en SIE-fil.
* Skapa en ny bokföring från namn, organisationsnummer och räkenskapsår.

## Ordlista

| Ord | Betyder |
|---|---|
| bokföring | Bokföringen för en juridisk person, i en egen mapp |
| samlingsmapp | Där cratchit håller reda på dina bokföringar |
| sökväg | Var en mapp ligger på datorn, till exempel `/Users/anna/Documents` |
| prompt | Raden där cratchit väntar på din tangent eller text |
