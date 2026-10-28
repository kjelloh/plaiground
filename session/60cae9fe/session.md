# Vibe code a Swedish book keeping app named cratchit - take 3 (Drop feed Claude)

```#60cae9fe```

## 20261028

* [Beräkning av mervärdesskatt vid öresavrundning](https://www4.skatteverket.se/rattsligvagledning/edition/2026.14/381778.html?utm_source=chatgpt.com)
    * The VAT amount reported to SKV shall be truncated (not rounded)
    * The truncated amount shall be booked on BAS Account 'öresavrundning' (3740)

DARN!! WE ARE STUCK!

* I totally fail to tell Claude how to generate a VAT Returns report from bas_to_momsrapport!
* I gets confused and when it analyses the existing books it conflates information all over the place?
* Now, It may be the case that the code Claude would generate could actually work?
    * But I am simply NOT SURE?

I AM SOOO TIRED!!

* Anyhow, I changed the format of bas_to_momsrapport to have anm out column and made box 49 map to out 1650, 2650.
* And Claude could make the changes to apply and parse this new file format ok.
* Claude now also shows only boxes aggregated FROM BAS accounts (49 mapped to out bas accounts excluded)
* Claude implemented calculation of Box 49 (and the signs seem legit for now)
* Vibe coded so all VAT boxes are truncated to whole SEK (round towards zero)
* Vibe coded a Vat Returns report voucher candidate mechanism.

At this stage I got the brilliant idea to ask Claude to compare our VAT Returns voucher candidate against historical SIE file (I have 11 years in a folder)

* It seems I now did things in the right order?
* I first narrowed the Claude task down to the voucher generation code.
* Then asked Claude to verify the mechanism against actual existing vouchers!
* FINGERS CROSSED this works!

Yes it did. Claude tells me a large bulk of historic VAT clearing vouchers matches the voucher we would have generated OK!

* Vibe coded creation of actual VAT Return clearing voucher ok.

Now I asked Claude to create a mapping from VAt returns form to eskd-file tag.

* [momsrapport_box_to_eskd](./cratchit/resources/momsrapport/momsrapport_box_to_eskd.csv)

In the chat with Claude I now discovered the VAT Returns boxes for income and sales had the wrong sign (Credit means '-' but the form seems to expect those to also be positive).

* So I made Claude flip the sign of boxes that aggregates from 'credit' accounts (accounts where '-' is the 'normal').

## 20261027

So now then we are ready to start generating the VAT Returns form document?

* We have the bas_to_momsrapport.csv table I manually created yesterday.
    * It is re-created from previous cratchit [Github cratchit thinking](https://kjelloh.github.io/cratchit/docs/thinking/thinking.html)
    * It is seeded (reversed engineered) from an exported 'kontoplan' in web based Fortnox accounting app.
* We have the mechanism to map postings to accounts in the books over a period to a VAT Returns document type.

Anyhow, Given the mapping from BAS accounts to boxes on the SKV 'Momsrapport', how do we iterate a mechanism that maps vouchers in the books over a period to a VAT Returns report we can send to SKV?

1. We can start by adding a user interface under 'Reports' to initiate 'Momsrapport'
2. Then provide some feed back to the user to inspect the generated document for correctness.
3. Then provide a second step to generate something we can send to SKV?
    * One form is the XML-file format 'eSKDUpload'

Lets ask Claude to go to work, shall we?

```
It is now time to initiate the 'Momsrapport' operation. I sugest a user option under '7  Reports of this year'. To begin with the option shall generate the VAT Returns document for a period. I want cratchit to know what VAT Returns period it must use (how often SKV expects a VAT Returns report). This setting should be in the project. Set it to 'Quarterly' for now. What can you propose?
```

* This made Claude come back with a reasonable proposal and good follow up questions.

```
1. Facts I shouldn't assume. As I understand it, SKV's periods are calendar month, calendar quarter, or yearly. Yearly follows the fiscal year (beskattningsår), not the calendar year. Is that right? And is "a quarter belongs to the fiscal year its last day is in" what you want?
2. Sign of the amounts. Should the boxes show amounts signed as in the books (sales come out as credit), or as the amounts you would type into SKV's form (positive)? The second needs a rule per box, which probably belongs with momsrapport.csv later. My suggestion for now is to show the book signs in Debit/Credit columns.
3. Writing the default. Should cratchit add vat_period = quarterly to existing projects on open, as proposed? Or should a missing entry just mean quarterly without touching the file?
4. Not VAT-registered. Do you want a none value now, or later?
```

* I went with don't break quarter for financial year. Ignore signs for boxes for now (later issue). Use quarter as default and VAT period none a possible value.

Claude implemented a reasonable user option to initiate 'Momsrapport'.

* It provided options to select possible quarters (default vat_period is quarterly)
* It reported back the BAS account postings (voucher referenced) for each box on the form.

I now asked Claude to show the VAT for each period option.

* I asked for the terms 'period' vs 'cleared' for calculated vs amount(s) posted to 2650, 1650.

Can I assume the 2650, and 1650 are NOT mapped to a VAT returns form box?

* [bas_to_momsrapport](./cratchit/resources/momsrapport/bas_to_momsrapport.csv)

    * They ARE mapped, but to box 49.
    * So the mapping assumes we enter the amount to clear to 2650 or 1650.
    * And then the mapping will take care of placing this amount on to the form box 49.

* So the algorithm to 'clear' VAT over a period goes through the books?

    * The 'report' boxes are calculated from BAS account mappings over the period.
    * The amount to clear is booked to 2650 or 1650.
    * Then we generate the VAT returns form again?
    * NO. This seems awkward?

It actually seems like the bas_to_momsrapport is the mapping AFTER the VAT has been cleared and booked to 2650 or 1650?

I asked Claude to refine the summary and use 'period', 'cleared' and 'charged' (although I may still preferr 'settled'?).

* Anyhow, I did a quick try and it seems to work ok.
* The options should be in the opposite order though (with option 0 being the next tp report to SKV)
* I think I like that we can select older periods too.
    * SKV actually allows us to 'correct' already reported VATs for earlier periods.

I now discover that the VAT for my next period to report is WRONG!

* It seems I have booked the wrong postings from the SKV statement file?

NO! Claude found out that I had manually registered an event from the SKV paper voucher with the wrong amount.

* So while eating Lunch I thought about adding some 'sanity checks' to cratchit to help me not do these stupid mistakes!
* It seems we should be able to make cratchit match the books against stashed statement entries?
    * All stashed postings should be in the books?
    * And no extra postings should be in the books?
* It seems the booked cleared VAT amount should match the calculated one``
    * If they differ we need to send in a new report?
    * Or ensure the discrepancy is carried over to the next report to send?
* OK, so the correct way to adress reporting the wrong VAT to SKV is to generate a new full correct report.
    * Can we implement the VAT report generation to always report the full correct report?
    * Also, if we correct the books we must NOT correct the vat-clearing-accounts until a new report is generated?
    * Otherwise there is no way to detect the previously generated report?

GOSH! All these complex book keeping intricacies?!!

OK, lets start with checking stashed statement files against the books for the period?

* No. That was easier said than done?
* Claude gave me different suggestions but ALL seems overly complicated and could give false negatives?
* I must be able to allow to book on 1630 with a different date than the one on a a statement?
    * This happens if I chose to book the 'momsbeslut' (VAT charge/settlement) and 'momsutbetalning' (pay) as the same voucher (this is quite usual)?
* We also have the crux of crossing the VAT period?
    * Could the error have occurred in the previous VAT period?
    * If so, then we should apply some more general reconciliation that allows for postings to differ slightly in exact date?
    * The order and the amounts must match though?
    * A fuzzy match?

I continued to chat with Claude but got insecure and confused.

* It seems it has implemented cratchit books to be able to carry a date for each posting?
    * This is fragile as SIE files do NOT do that!
    * Postings are implicitly assumed to happen date of the voucher (event)
    * So maybe I need to ask Claude to remove this functionality?
    * It won't survive exchange with external tools anyhow (SIE file round trip)
* Claude also explained that there may be errors in how it assumes covered dates for imported statement files.
    * I may have to ask claude to ONLY use the dates of the actual postings?
    * And treat optional open and close Saldo as a way to extend the covered range?

GOSH! Do I really benefit anything from using Claude?

* I have now fikat and gone for a run.
* I now decide to let this idea of reconciliation to simmer a bit more.
* I DO in fact already get an indication that something is wrong from cratchit.

So AM I now able to fix the A11 voucher?

* I ned to be able to cancel it (it is the wrong amount AND the 'same' as A14)

Well, NO. I don't see an easy way to cancel A11 (nor edit the postings?)

* So maybe first we should add the operation to edit and/or change it?
* In Fortnox 'Ändringsverification' seems to create a new voucher that cancels all postings on the selected one? 
    * What is the english name for this?
    * Should we add this operation?
* In Fortnox 'Ändra' seems to open an editor where we can edit existing postings and add new ones?
    * What is the english name for this?
    * Should we add this operation?

Yes, it is time to add these operations on vouchers!

* The 'Ändringsverification' is straight forward.
    * Initiate a new voucher with postings from the selected voucher reversed.
* The 'Ändra' is more elaborated.
    * We need to comply with 'bokföringslagen'.
    * Maybe a short-cut is to comply with the SIE file format?

So I have now read the SIE file format.

* It seems we may optionally support the notion of 'Tillagd transaktionspost' (Added post)
    * This post shall be represented by an '#RTRANS' element in the SIE file voucher '#VER'
    * But If we do we must still produce also a normal '#TRANS' (normal post)
* It seems we may optionally support the notion of 'Borttagen transaktionspost' (removed post)
    * This post shall be represented by an '#BRTANS' element in SIE file voucher '#VER' and NO '#TRANS' for this element shall be provided!

GOSH!!

* Do I really need to implement this behavior?
    * To implement I need to track #TRANS from SIE file and ensure they get mutated only with #BTRANS (replaces previous #TRANS in imported #VER) or the pair #RTRANS + #TRANS (extends postings in #VER in imported SIE file)
 
 Do I dare let Claude get a go at this?

* I now found out how the 'Correct It' actually works (confusing but it is there)
    * I asked Claude to modify it.
    * Now 'Correct It' enters 'new voucher' mode with suggested text, amount and date of corrected voucher
    * From here the flow is the 'normal' new voucher flow (itself still confusing though)

Anyhow, I was able to create a voucher that canceled the faulty A11.

I was now tempted to also add 'delete last voucher in series'.

* But this is not allowed just like that.
* It seems we should add some mechanism so that cratchit keeps track of whether a posting is 'committed'?
    * A posting is committed if it comes from an imported SIE file (knowledge about its existence is shared outside cratchit!)
    * I can imagine other ways a posting is committed.
    * If the voucher is printed.
    * If the voucher is exported to an SIE file
    * If it is counter-posted by a correction voucher?
    * Do I dare to go down this rabbit hole?

NO! GOD, This was VERY VERY tricky!!

* I have no insight into what architecture Claude as actually built?
* This makes it hard for me to ask Claude in a way so that I can determine from its answers if the change is 'safe' (would work? Could break anything?)

I need to take a break and sleep on it!

* Either I open up so cratchit always allows deleting the last voucher in a series
    * It seems like a common operation allowed in other accounting programs?
    * Fortnox allows it.
* Or I ignore that for now to brute-force cratchit to report VAT, close the books, Create ÅR and report K2 tax returns to SKV? 


## 20261026

So maybe we should now go straight for the VAT Returns report?

* We need some form of representation (specification) of the VAT Returns report as defined by SKV?
    * Can I ask Claude or should I dig up my previous research implemented in cratchit::zeroth?
    * It seems cratchit::zeroth uses a CSV snippet from somewhere (unclear form where)?

```cpp
char const* ACCOUNT_VAT_CSV = R"(KONTO;BENÄMNING;MOMSKOD (MOMSRUTA);SRU
3305;Försäljning tjänster till land utanför EU;ÖTEU (40);7410
3521;Fakturerade frakter, EU-land;VTEU (35);7410
// ...
``` 

I tried to browse SKV web site for actual machine readable specifications?

* It is a MESS!
* I fail to find any indications about where to find technical specifications?
* AHA! There is something called [Utvecklarportalen](https://www7.skatteverket.se/portal/apier-och-oppna-data/utvecklarportalen)?
* There seems to be an API for VAT Returns report [Momsdeklaration 1.0](https://www7.skatteverket.se/portal/apier-och-oppna-data/utvecklarportalen/api/momsdeklaration/1.0.25/Översikt)?
* I seem to have found a pdf that defines the REST API for the VAT Returs report [](https://www7.skatteverket.se/portal-wapi/open/apier-och-oppna-data/utvecklarportalen/v1/getFile/tjanstebeskrivning-momsdeklaration-v1-0/pdf/1.0.9/Tjanstebeskrivning-Momsdeklaration-v1.pdf)

    * I think the section '4.1.1.3 Momsuppgift Det underlag som ska läsas in och kontrolleras' defines cross references between API, Paper report boxes and XML file for the VAT Returns report?
    * The table in '4.1.1.4 Beskrivning av API:ets momsuppgifter' seems legit?
    * But the pdf does to enable copy-and-paste that preserves the table structure *SIGH*
* I seem to remember there is a zip-file to download with SKV forms that I used for cratchit::zeroth?
    * Yes, it is [Teknisk information om filöverföring](https://www.skatteverket.se/foretag/inkomstdeklaration/forredovisningsbyraer/tekniskinformationomfiloverforing.4.13948c0e18e810bfa0cca8.html)
    * But this is for files based on INFO.SRU and BLANKETTER.SRU
    * The VAT Returns report is an XML file that can't be provided with these SRU-based files?
* 
    * Google AI states.

    ```
    • Period 1 (P1): Företag med bokslut som slutar under perioden januari till april.
    • Period 2 (P2): Företag med bokslut som slutar under perioden maj till juni.
    • Period 3 (P3): Företag med bokslut som slutar under perioden juli till augusti.
    • Period 4 (P4): Företag med bokslut som slutar under perioden september till december (här ingår det vanligaste kalenderåret).
    ```

    * So my ITfied that ends its year April 30:th is period P1 then?

* AHA! I now seem to have found information about XML files at [Testtjänst och teknisk beskrivning](https://www.skatteverket.se/foretag/skatterochavdrag/kontrolluppgifter/testtjanstochtekniskbeskrivning.4.233f91f71260075abe8800073614.html)

I browsed around for a while but got nowhere! SO I tried asking Claude.

```
I now want to start designing the architecture to create VAT Returns reports to SKV. But it is quite vague where I can find some reliable specs to use? I have a previous implementation based on a csv-snippet in a constant 'ACCOUNT_VAT_CSV' @/Users/kjell-olovhogdahl/Documents/GitHub/cratchit/src/fiscal/SKVFramework.*. But I seem to have forgotten where I got this csv from. What can you do to help me find the source?
```

I GIVE UP!

* Claude is willing to help and sounds ensuring.
* Still, I can't trust what it proposes and it goes down elaborate rabbit holes that I find over complicated.

I seems I have found the SKV 'specification' on how to fill in the VAT Returns report?

* It seems to be described at [Fylla i momsdeklarationen](https://www.skatteverket.se/foretag/moms/deklareramoms/fyllaimomsdeklarationen.4.3a2a542410ab40a421c80004214.html)

It seems I have found the specification for the XML-file format for the VAT Returns form to SKV.

* It is described at [Lämna momsdeklaration via fil i e-tjänsten](https://www.skatteverket.se/foretag/moms/deklareramoms/lamnamomsdeklarationviafilietjansten.4.2fb39afe18dabf1e4d223cc.html)

    * The file seems to have three sections

    ```
    Inledning (Header)
    Momsinformation
    Avslut
    ```

    * The mandatory rows for the headers seems to be defined as:

    ```

    Rad 1       
        <?xml version="1.0" encoding="ISO-8859-1"?> 
        Måste finnas med.
    Rad 2       
        <eSKDUpload Version="6.0">                  
        Måste finnas med.
    Rad 3       
        <OrgNr>556000-0175</OrgNr>                  
        Den momsregistrerades person-, samordnings- organisations- eller representantnummer. Ange numret med 10 siffror enligt formatet xxxxxx-xxxx, med bindestreck. Organisationsnumret i exemplet är påhittat.
    Rad 4   
        <Moms>
        Måste finnas med.
    Rad 5 
        <Period>202501</Period>
        Redovisningsperiod. Använd följande format.
        Månadsredovisning: År och månad, 202501 (januari 2025)
        Kvartalsredovisning: År och den sista månaden i aktuellt kvartal, 202503 (första kvartalet 2025)
        Helt beskattningsår: År och den sista månaden i beskattningsåret, 202512 (om beskattningsår motsvaras av ett kalenderår motsvarar exemplet januari–december 2025).
    ```

After all this chatting with Claude and browsing myself - I GIVE UP!

* I think I have to manually write the seed a mapping from BAS accounts to VAT Returns form box nos?

    * I use the web form I get presented when using SKV VAT Returns report web interface.

I now manually created mapping csv-documents.

* [momsrapport](./cratchit/resources/momsrapport/momsrapport.csv)
* [bas_to_momsrapport](./cratchit/resources/momsrapport/bas_to_momsrapport.csv)

I now asked Claude to implement code that applies bas_to_momsrapport over vouchers for a period and creates a document type holding the accumulated vat returns form amounts.

At this stage I could not help myself and finishes the day by asking Claude to refactor the whole UI to be totally numbered-option-based.

* I felt I needed to do this before adding more to the UI.
* Maybe if I do it now, Claude will get better at picking up the design and provide more consistent UI suggestions?
* In any case, I like to navigate with number keys only (It worked for cratchit::zeroth first and second)?

And it seems Claude was able to do it?

* I did a quick try and I could not spot any errors?
* It is still inconsistent at some places where Enter on an empty input goes back (where '-' is the one I would like everywhere).
* And it enters 'line input mode' instead of using self administrated buffering (so it can't pick up '-' on empty buffer)

So now I made Claude make line-input act on '-' if the buffer is empty (one key exit from buffered input modes ok)

## 20261025

I have now decided to try and complete booking the events for my companies next VAT returns report and make notes on what works and what may need fixing?

* It would look nice I think if Debit/Credit for postings where listed in separate columns?
    * Like Debit to the right and Credit to the left?
    * Maybe also with a saldo line for Debit and Credit as a 'current balance' row?
    * In edit mode the saldo row can serve also as 'what is left' for next posting?
* The 'waiting list' mode should be retained while editing I think.
    * So when an entry is made into a voucher, the remaining waiting list is shown.
    * So the user can just continue to pick from the list until it is empty.
* I still think it would be nice to be able to enter a financial event with an Euro amount?
    * We know for now that it will be transacted in SEK still
    * So the event / voucher amount needs to be carried along until it is no longer required.
    * Maybe have the heading of the event be amended with the Euro amount in the final books?
    * That could at least give a nice feed back for reading the events (although no actual official amounts are booked in Euro for now)?
* We may need to be able to adjust date and amount when turning a waiting event into a voucher?
    * In this way the user can adjust any errors in entering the event?

OK, I hit a show-stopper when trying to enter a return (credit) invoice

* The pick template works (It shall be registered as an invoice in series D ok)
* And the BAS account 'leverantörsskulder' cratchit suggests is also correct.
* But the sign is wrong! This credit invoice means to Debit 'leverantörsskulder'
* I also was not able to 'undo' when I managed to post to the first suggested account.
* I also do not know how to exit out of the 'add posting' mode (go back to the waiting list)

So it seems we need an enhanced user interface while editing the postings of the event?

* We should be able to pick previous (or any) posting?
* We should be able to change a selected posting
* Maybe a full fledge 'edit postings' UI where we can both edit and remove a posting?
* We need a way to exit back to the waiting list

What could be a first design for such an enhanced 'edit post list' UI?

So I asked Claude and after some back and forth ti implemented a solution.

* But I did NOT like the solution.
* So I reverted and tried again.
* The solution was partly my fault and partly Claude just not piecing the UI together in a logical way.

OK, I now realize I have tackled this 'credit invoice' all wrong.

* For a credit invoice we need to cancel the 'cost'
* But depending on whether the original invoice is payed or not we need to do different things.
* If it is not yet payed, then we shall cancel the debt to the vendor.
* If it is payed, then we should book an asset (place a return liability on the vendor)

This is a more elaborated. So what UI can we imagine that works sufficient enough for now?

* Maybe provide some hints on existing postings in the book for the amount?
    * In that way the user can see what is in the books and draw a conclusion on how to deal with the credit invoice?

What does Claude have to say about this now? I placed an open question to see what Claude knows about return credit invoices in book keeping. 

* It suggested to store signed amounts in events (vouchers)
    * But I am not ready to open that can of worms?
    * The sign in double accounting is tricky (I '-' always Credit? And if so, is '-' actually 'opposite what is normal?)
    * I think I want to pass positive amounts around for now?
* I discovered that events are not sorted in Date order!
    * We need to fix this.

So I think we go for a smaller simpler fix for now.

1. Make the waiting events list in date order (oldest on top)
2. Allow the user to 'flip' a template
3. Add some 'hint' about existing postings in the books to the template account or any with the same amount.

That could serve as a good base fo the user to get guidance towards creating a correct credit invoice?

So I now decided to drip-feed Claude to get through these changes with some confidence in the cratchit behavior.

* Cratchit now manages the waiting list in date order (oldest first)

I continued with small changes.

* Cratchit now formats posting amount into debit and credit column.
* Now creates f a full listing of postings from template. 

At this stage I get a string intuition that Claude gets very confused about the UI we now have?

* I feel I have to pick really small changes to have it propose an acceptable design.
* I asked for a '-' navigation from 'post' all the way back to the waiting list.
    * I even allowed Claude to go ahead.
    * Then I got cold feet and aborted.
    * It is hard now to know how to prompt to get a design I can trust?

I am now starting to feel what I heard others working with agentic coding has described.

* The fun part of implementing the code and get a reward for each bit that gets working is taken away.
    * Watching Claude do the 'fun' part is boring an unsatisfying.
* I also feel out-of-touch
    * When I don't tell the computer with code myself, I feel weary that the code does not in fact do what I want?
    * Also, I lack the insight to catch things I have not thought about (Claude implements and I get to test a back box)

I challenged Claude to implement a 'merge event' mechanism to identify 'same event' from events on the waiting list.

* At first it sounded good.
* But when I tried what Claude had implements I discovered the UI was basically no help at all.
    * It had provided an 'm' option
    * Pressing 'm' asked for an event to keep.
    * Then an event to merge with it.
    * No hint or guide to candidate events?
    * Kind of brain-dead UI?

* So I reverted the changes and went fresh with a new prompt.

```text
I now want cratchit to help with identifying 'same event' from the events in the waiting list. For an event to be 'the same' it must be the same amount. And the events can not be further apart than bank days or over a week end. That is because in sweden and using 'fakturametoden' an invoice and following payment is always separate events. I want the listing to change so that 'same event' candidates are grouped in the listing. If possible, I would also like the mechanism to try and pick up if the event is already in the books. What can  you propose?
```

* That prompt gave no good design. I tried a new one.

```
I now want cratchit to help with identifying waiting events that are in fact the same event. When in 'edit waiting list' mode I want an option with a good name for finding 'same event' and pick to make 'same'. I also want this mechanism to suggest that events may already be in the books. I want 'same event' to be possible only if within days. Define a time window to apply for now. I want the result of the check to be a new list where the user can pick and apply (edit) suggested found 'sameness'. What can you propose?
```

* This time I got something that I liked.
* But then I missed to answer Claudes questions before I started and decided to abort and start again.

This time I got something that seems to work (quick try seems ok)?

Some time later and I have now actually registered ALL my current vouchers using this vibe coded cratchit.

* I will now sleep on it and maybe redo it from scratch to pick up any more UI tweaks that I need to get done before I go on with VAT return resport, closing the books, generate voucher for printing etc.

## 20261024

I have now decided to 'brute force' functionality by Vibe coding and don't care too much about code structure or design.

So what's next?

I asked Claude to vibe code a 'pick voucher template' as an option when editing and or posting a financial event.

I tested the 'pick template'.

* It seems to work sufficiently?
* I think what I now need most is a way to remove vouchers I have posted?
    * I need this anyhow for live application of cratchit.
    * And this will enable me to test and revert on my current example data.

So I asked Claude to implement view and edit existing vouchers.

```text
Both tasks are done: you can view the vouchers of a fiscal year, correct a voucher you entered in cratchit, and start a new voucher from any viewed one. 
```

I made a quick view run-through and it seems to work just fine.

* I now tried the following prompt:

```
Time for next task to parse account statements into something to use as base for creating appropriate events and vouchers. I have added a larger set of account statements in _resources/_from_bank_or_skv. I think I want a parser that creates some form of 'account statement document' in memory. I think I want it to hold only valid debit and credit posting to the account. A valid account posting shall have at least the Date, posted amount and a caption text. The account statement file may list the postings in forward or reverse order. It may provide an optional saldo amount. It would be nice if the parser is able to identify the Date and the posted amount on column content only. That is, we expect a valid row to contain only one pure date column and at least one pure amount column. Maybe we can assemble a text by somehow assemble identified text columns into one text? If possible, I want the parser to verify Saldo amounts by running a row-by-row accumulation to verify that posted amounts add up to the running saldo amount where there is one. I want to leave using the account statement document for now to first see what we can design? What can you propose?
```

* Now Claude also picked up my spec in this session document (which turned out to be a good thing)!
* It cae back with questions about single line statement files, encoding, text joining and opening closing saldo.
    * I asked it to accept single line if there is only a single amount and date field (assuming this should always mean it is a debit/credit entry?)
    * I asked it to reject the file if no BOM and UTF8 did not work (seems safe for now?)
    * Also, fall back to any Latin-based encoding may not work world-wide?
    * I mean, For slavic languages, different scandinavian langues etc., we can assume we know what sub-set of Latin we are dealing with?
    * I asked for text joining with ','
    * I asked for identifying open and close balance by matching running posted amounts against them (open saldo + running posts = close saldo)

I have now vibe coded 'import account statement' file(s)

* There is a UI from project and year (pruned to that year)
* Imported postings are stashed in a statements file in dot-folder named after start-date

And now I have vibe coded 'statement entry to waiting event'

* I can view the unbooked statement entries
* I can turn a statement entry into a waiting event

The UI is very cluttered compared to my previous cratchit. But maybe that is what we need. And I can always clean this up later?

* We have statement::Entry to represent an entry in an account statement
* We have books::Posting to represent posting to a BAS account

## 20261023

It is now afternoon and I will try to make Claude implement an account statement file reader.

I am not yet sure if I want to create an 'account statement document' or directly to plain BAS account postings?

Anyhow, I edited the following specification by going back and forth with Claude.

* The reader shall turn account statement rows into BAS account postings
* Any consolidation of postings into events and then vouchers are separate later operations.
    * The reader shall only be concerned with creating valid BAS account postings.
* The reader shall be able to read csv-files in at least UTF8 (using existing character set transcode mechanisms for simple detection)
* It shall be able to consume any BOM in the file (so it does not turn up as garbage in read rows) 
* It shall be able to detect and parse a file using separators from a small set (and implement at least one default one for now that works on examples I provide from Nordea and SKV)
* It shall identify a continuous range of account statement rows that represents an account debit or credit.
    * All rows must match the criteria of a valid debit or credit statement.
    * All rows must share at least one Date column, one amount column and one text column (to be used as posting caption)
    * All date values must be pure (date found in text counts as text, not a valid date)
    * All amount values must be pure (amounts found in text counts as text, not a valid amount)
    * An invalid row terminates the range
* The reader shall be able to read an extra amount from some rows to handle optional Saldo amounts.
* The reader shall apply an algorithm to determine the posted amount column from any saldo amount column.
    * If only one column carries an amount for all rows then this must be the posted amount.
    * It shall accumulate posted amount over the range both forward and backward to detect row order of postings
    * It shall accumulate suspected posted amount and compare with suspected saldo amount (both ways if not yet determined)
    * It shall apply these checks row-by-row (all rows must comply with the rules)
* If no conflicts are detected the algorithm shall accept the identified columns content.
* If rows order and amount types are confirmed it shall transform them into potential BAS account postings.
    * Initially it shall use a BAS account that is either nullopt or a 'observe' BAS account for later assignment.
* The reader shall NOT try to determine meta-data that is not explicitly in the file.
    * None of e.g., The bank name, account name, bank account number, SKV account number etc. shall be assumed to be in the file
    * Explicitly, linking postings in the file to a BAS account is a later separate step and NOT a task for the reader.
* The reader shall NOT try to find separate open and close balance rows
 


Today I decided to ask Claude to refactor the current design so we can treat a 'voucher' as having two parts.

* One part is the Heading, Amount and Date (The overall thing about a financial event)
* The other part is the aggregate of postings to BAS accounts for this event.

Claude came back with a design and refactoring tasks that I found reasonable. So I accepted and let Claude do its thing.

* The term 'Transaction' (inspired by SIE entry #TRANS) is now refactored to 'Posting' in code
    * This makes sense as it is in fact not a transaction (not a from-to thing) so 'posting' is better.
* The term Voucher remains the complete registered financial event in the books (Matches SIE #VER)
    * So I think we will end up with Voucher being an Event + Postings?
    * YES. Claude seems to confirm this.

```text
Step 1 is done: vouchers are now built from an Event and its Postings, and the UI works as before. The build has no warnings and all 343 tests pass. I also ran it in a scratch folder: a Fortnox sample imports, importing it again gives "Nothing to import", and the voucher report looks as before.
``` 

I went ahead and made Claude implement the 'enter events' UI also.

```text
Step 2 is done: you can now note events like invoices and receipts at the year prompt with n, and post or delete them later with w. The build has no warnings and all 347 tests pass. I also ran it in the app on a copy of your Fortnox sample: I noted two events, posted one as voucher D 27, and deleted the other.
```

I now tried entering events.

* It would be nice if we remained in 'notes' state so we can continue to enter events.
    * Remove the 'y/n' confirmation (show the waiting list)
    * Remain in 'notes' ode so the user can enter a new one directly
* It would be nice to enter an event with a Euro amount (e.g., Anthropic Receipt)
    * Store amount with currency
    * Resolve in posting to BAS accounts (E.e., withdrawal from swedish bank account in SEK)?
    * Allow to create SIE voucher (should be OK as SIE voucher has no amount of its own?)
* Make empty enter in 'event' list waiting
    * This makes it easy for user to check where in the list the are (what has been entered so far)
* We actually would like an Event to carry negative amounts (e.g., credited invoice)
    * I had an invoice credited and I think it would be nice to have the amount sig be carried along?
    * On the other hand, it will be handled in BAS account posting anyways?
    * But maybe a negative amount makes it easier for the user to get Debit/Credit correct?


## 20261022

I wonder if I still lack trust in the vibe coding process? What happens if I just go for it? Still task-by-tak though?

* So I decide if a task is done or needs tweaks.
* I decide what the nxt task should be.
* But I do NOT care about what the code looks like?

Do I dare to do this?

* I still need to trust cratchit to perform book keeping and reporting correct!
* I can accomplish this by asking for cratchit being able to show reports about what it has done.
* I can integrate required book keeping information into the use manual (help file)?
* In this way I can read the requirements and compare with what cratchit does?

Ok, enough thinking and second guessing. What is next?

* Read in an SIE file!
* Cratchit now needs an internal representations of the books.
* How much do I need to tell Claude?
* Should cratchit stash a clone of the imported SIE file?
* Should cratchit remember what was imported to keep track of changes compared to the imported books?

I asked Claude to implement SIE import with user interface, parser and some open hints about if to stash the import and use sie.se and bas.se for specs.

* It came back with the proposal to begin with the SIE file lexer.
* But it also laid out task steps towards SIE import, internal book update and checks before accept.
* I asked me about overlaps, support for SIE 5 etc.
* I actually felt reassured to go ahead from the questions and what it seemed Claude (and the model) understood about the goal.

Claude came back with some interesting insights.

* Fortnox SIE file provides no checksum (fragile SIE file content?)
```sh
- The samples confirm a few things the parser will need to handle:
  - #RTRANS is followed by a duplicate #TRANS, as I suspected.
  - Fortnox also writes #BTRANS, #PSALDO, #DIM and #FNR.
  - There is no #KSUMMA, so the checksum check is only possible when a file has one.
```

* Claude asked.

```sh
Decision for you: CP437 maps every byte to a character, so decoding never fails and there is no failure count to abort on. How should cratchit decide that a file is really in another encoding? Two options:
- abort when the file is valid UTF-8 with multi-byte characters;
- abort when CP437 decoding gives box-drawing or graphics characters, which don't belong in bookkeeping text.
```

Claude have now defined a list of tasks to implement SIE file import.

```

Seven tasks remain in the SIE import plan, numbered 20–26 in progress.md:

20. SIE document, part 1: company details (#FNAMN, #ORGNR, #RAR, #KPTYP and similar) and the chart of accounts (#KONTO, #SRU, #KTYP), read from the lexed lines into the SIE document.
21. SIE document, part 2: opening and closing balances (#IB, #UB, #RES, #PSALDO), vouchers (#VER with #TRANS, #RTRANS, #BTRANS) and dimensions (#DIM, #OBJEKT). In the Fortnox files each #RTRANS is followed by a duplicate #TRANS, which has to be skipped.
22. File checks: the checksum (#KSUMMA) when the file has one, vouchers that sum to zero, accounts that exist in the chart, and opening balance plus transactions equal to the closing balance.
23. Import dry run: i at project> lets you type a file path. Cratchit then shows a summary, the check results and any encoding alert, and changes nothing. This is the first step you can try in the app.
24. Books model: cratchit's own representation of the books, and the files it is saved in under the project's .cratchit.
25. Import:
    - You confirm with y/n, and the books are updated.
    - The first import sets the org number and other company details. A later file that doesn't match them is an error.
    - New financial events are added; existing ones are never changed.
    - The file is stashed in the project with a record of the import, the import is logged, and the manual is updated.
26. Reports: saldobalans (trial balance), a voucher list and an account ledger, so you can compare cratchit's books with Fortnox.

Later, outside this plan: checking the chart of accounts against BAS, exporting SIE (and testing that an import followed by an export gives the same file), and SIE 5.
```

At this stage I went for it and told Claude to implement all remaining tasks in one go. ANd ask only if critical decisions needs to be made by me.

* It came cak with questions about miss-matched in and out balances in SIE files.
* It detected that input balance for 2026/2027 did now match output balance for 20205/2026.
    * I suggested cratchit should warn and use its own in and out balances if it has them
    * And alert in log and to the user about the discrepancy but ignore and use contained vouchers.

Now when I think about it.

* If the in-balance in an SIE-file differs from the out-balance in a previous year imported SIE file, then this means the SIE file snapshots differs.
    * But we don't know where the difference is?
    * Cratchit should always treat its own books as the 'truth'
    * And update this truth from SIE files only if there is NO conflict!
    * We can later add a 'force' flag to re-synk cratchit with an external truth.
    * But for now SIE file import only allows for new financial events - NOT mutations!

 
 Claude have now implemented SIE import with user interface and persistent storage.

 * The user interface works (but is over complicated!)

 ```text
 project> 
Keys at project>:
  i  Import an SIE file
  r  Reports
  l  Show the log
  -  Back to workspace>
  h  Help (the manual)
  q  Quit
project> i
Type the path of the SIE file to import. Tab completes a folder or file name, Backspace erases,
Enter accepts. Enter on an empty path goes back.
path> /Users/kjell-olovhogdahl/Documents/GitHub/plaiground/session/62c6984e/_sie_in/TheITfiedAB20261019_105220.se
SIE file /Users/kjell-olovhogdahl/Documents/GitHub/plaiground/session/62c6984e/_sie_in/TheITfiedAB20261019_105220.se
  Read as      CP437
  Program      Fortnox 3.62.17, generated 2026-10-19
  Company      The ITfied AB, 556782-8172
  Fiscal year  2025-05-01 - 2026-04-30
  Accounts     501: 501 new, 0 changed
  Vouchers     116: 116 new, 0 already in the books
Notes:
  - The project gets organisation number 556782-8172 (#ORGNR)
  - The project's name changes from "ITfied AB" to "The ITfied AB" (#FNAMN)
Import it? (y/n) y
Initiated .cratchit/books/books.txt in /Users/kjell-olovhogdahl/Documents/GitHub/plaiground/session/60cae9fe/ITfied AB
Initiated .cratchit/books/accounts.csv in /Users/kjell-olovhogdahl/Documents/GitHub/plaiground/session/60cae9fe/ITfied AB
Initiated .cratchit/books/dimensions.csv in /Users/kjell-olovhogdahl/Documents/GitHub/plaiground/session/60cae9fe/ITfied AB
Initiated .cratchit/books/objects.csv in /Users/kjell-olovhogdahl/Documents/GitHub/plaiground/session/60cae9fe/ITfied AB
Initiated .cratchit/books/2025-05-01/opening.csv in /Users/kjell-olovhogdahl/Documents/GitHub/plaiground/session/60cae9fe/ITfied AB
Initiated .cratchit/books/2025-05-01/vouchers.csv in /Users/kjell-olovhogdahl/Documents/GitHub/plaiground/session/60cae9fe/ITfied AB
Initiated .cratchit/books/2025-05-01/transactions.csv in /Users/kjell-olovhogdahl/Documents/GitHub/plaiground/session/60cae9fe/ITfied AB
Updated .cratchit/project.txt in /Users/kjell-olovhogdahl/Documents/GitHub/plaiground/session/60cae9fe/ITfied AB
Stashed the file as .cratchit/imports/2026-10-22_1_TheITfiedAB20261019_105220.se in /Users/kjell-olovhogdahl/Documents/GitHub/plaiground/session/60cae9fe/ITfied AB
Imported 116 vouchers into fiscal year 2025-05-01 - 2026-04-30 (0 already in the books).
project> 
 ```

What are ALL THOSE new project files!!

At first I thought I should ask Claude to clean this mess up.

* On second thought I imagine I can leave the result as is and just carry on?
* This is Vibe Coding so we can take this as an exercise to see how agentic coding behaves?

So next we can now implement actual accounting and accounting support?

* Do we need to somehow base this functionality on BAS specification(s)?
* Maybe to provide BAS accounting help in the manual?
* Do we need to store our own machine readable 'account plan'?
* If so, how do we use account information from SIE files with BAS accounting specification?
* Can they clash?
    * If so, what do we doo?

Finally, I actually already know we will later need to know how class 8 BAS accounts map to their types (asset, liability, cost, income).

* Is there a BAS specification from which we can extract account types for each BAS account?

So I asked Claude to investigate and the answer is annoying.

* It seems BAS charges 4000 SEK ex VAT for a json-file.
    * And this json-file is then only licensed for won use (not copy, share or sub-license)
* And the free pdf-s are fragile to read (says Claude)

OK, So where are we?

* It seems BAS free specifications are in fact worthless?
* At best, they can be used to get the names of BAS accounts?
* But no meta data about counter accounts, account types etc.
* So it seems for now totally pointless to try and download and parse them?
    * Also, previous cratchit::zeroth also did NOT extract any meta data from these BAS specs.
* Does the SIE file in fact specify BAS account names?
    * YES!

But it irritates me that I fail to get any specs on BAS account class 8 account types!

* I can buy the book "Svensk Redovisning. Bokföringsboken 2026" and read about it?
* I could then manually create a meta-data map for BAS accounts?
    * I may then be able to manually define the account types for BAS account class 8 accounts?

OK, I bought it as e-book.

* It does NOT state the account types in account class 8!
    * Do I have to deduce the type from the account name?

I have now studied BAS vs SRU vs INK2.

* It seems the information in the SIE-file is self contained and sufficient?
    * Each BAS account is mapped to SRU code
* And maybe the INK2 fields are always defined by their SRU codes?
    * So as long as BAS vs SRU is correct, then INK2 from SRU will also be correct?

Also, for account class 8.

* The BAS Account to SRU code is sign dependant!
    * So if the BAS account sums to a positive value it links to one SRU code
    * But if it is negative it links to another SRU code!
    * E.g., BAS account 8810

    ```
    7420    3.21    Aterföring av periodiseringsfond    8810 (Om netto +), 8819
    7525    3.22    Avsättning till periodiseringsfond  8810 (Om netto -), 8811
    ```

    * 8810, if saldo positive, accumulates to SRU 7420 (INK 3.21)
    * 8810, if saldo negative, accumulates to SRU 7525 (INK 3.22)

So how does the SIE file (My Fortnox export) handle this?

* In Fortnox BAS 88100 seems linked to SRU 7420 ?!
* And YES, in _sie_in/TheITfiedAB20261019_105324.se the mapping is '#SRU 8810 7420'!

HOW ON EARTH IS THIS SUPPOSED TO WORK??!!

* AHA! You are supposed to use explicit BAS 8819 or 8811 as an accountant ?!!
* YES. In SIE file '#SRU 8811 7525'!

OK, maybe time to see if we can implement some basic accounting?

* It seems Fortnox applies (suggests?) a list of pre-defined series to enter vouchers?

```
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

After some forth and back I made Claude implement accounting vouchers.

* I now discover we lack 'open fiscal year' (should list option 0,1,2,... for one key select current, previous, before previous...)

I made Claude fix that.

**Todo SIE Import, New voucher, VAT support, Template voucher from history**

* I now discovered I would like import SIE file to list available SIE files if the path is a folder and I press enter
    * I would like each SIE file to be read for financial year and show at least that in the options list
    * A fancy 'latest of same year' filter would be nice to.
* I think new voucher y/n after Heading Amount Date is redundant and can be removed.
    * I imagine if the input could not be parsed we can guide the user to edit?
* The Series selection can be clearer by listing the default with a '*' prefix?
* When choosing posting with a word or number and get account listing we can enhance the user interface.
    * We can 'drill down' with more user input (match only current list moving on)?
    * We can provide bullet list with single letter from first word of listen account names?
    * We can drill down from user entering part or whole account number (reduce list to matching input number)?
    * Also, entering a number should math from beginning of account number (not 'contains' as now )
* We need VAT support
    * Can we detect if we select a VAT account?
    * Can we then also detect VAT percentage?
    * Then we can propose a VAT
    * Then maybe this is overkill as invoices always states VAT?
* Cratchit did not understand to switch to Credit for VAT
    * If Debit matches input amount, Cratchit can switch to assuming remaining Credit to be posted?
    * The opposite should also be true?
* I want some hints to what accounts have been used in historic vouchers of same type?
    * Provide the user with an initial list of 'templates' to mirror account postings from?
    * On empty enter, provide a list of accounts posted to in vouchers of 'same form'?
* I can't, don't know how to (and get no help on how to) exit from 'post' state?
* Ctrl-C exists without safety net!
    * Is it possible to catch Ctrl-C and auto-save?

## 20261021

Now it is time to introduce the scaffolding for character set encoding and decoding.

* I want character set transcoding to be based on a stack-like buffer with a 'push' operation.
* Two buffers are required to transcode from one character set encoding to another.
* One character set to Unicode buffer that accepts push of bytes from the input character set encoding and produces Unicode code code points as they are available.
* One Unicode to character set buffer that accepts push of Unicode code points and produces an std::array of multi byte encoding in the encoded character set.
* The character set to Unicode buffer 'push' returns an optional Unicode code pointer to signal when there is one available in the buffer.
* The Unicode to character set buffer 'push' returns a single byte or an std::array as required to return single or multi byte encoding for the target character set encoding.

I want to try this architecture out for the CP437 character set.

* I want the code places in namespace 'charset::cp437'
* I want to first implement CP437 to Unicode based on this design.

What can Claude propose as an implementation for this design?

Claude implemented CP437 to Unicode buffer for me.

* Now I want Claude to take this design an implement Unicode to UTF8 for me.

I have now iterated some task steps and think I have a character set transcoding architecture in place.

* I cleared Claude and asked fresh about some clean boundary between internal character set string encoding and terminal io.
* And Claude came back and said - Not so!
* It then proposed several aspects to attend to.

So I decided to continue the 'vibe coding' approach and let it do its thing as it found 'best' based on my asking for the attention and design goals.

Now I have reviewed the change Claude proposed and I do no longer agree.

* It identified a problem with UTF8 to Unicode where a broken sequence may result in one unknown character (the broken escape) and one potentially valid one (the sequence breaking byte).

```sh
The problem is narrower. It's the case where the byte that reveals an error is itself a valid character:

bytes:   E2      82      61 ('a')
push:    nothing nothing ?

After E2 82, the decoder is waiting for a third byte. Then a arrives. It isn't a continuation byte, so two things are now true:

1. E2 82 was a broken sequence, so the output owes a U+FFFD.
2. a is a perfectly good character, so the output owes an a.
```

* But on second thought I do not believe returning both the unknown character and the breaking one as a valid one is the way to go?
* I think I will revert Claudes fixes and ask for a clarification in code that in this case the breaking sequence shall consume all three into an unknown character?
* I mean, We AER assuming it is UTF-8. So if we find an invalid sequence, then the whole sequence can be mapped to an unknown char (assuming the breaking char is itself a valid char is NOT automatically a good resolution)?

I started fresh again and asked Claude for this UTF8 to Unicode solution.

* It now again commented on throwing away a valid character that breaks an escape sequence.
* It states the Unicode standard recommends to produce two Unicode code points invalid-char + valid-char.
* I still disagree based on my argument above.

I think I will try to make Claude keep the breaking char if it is a new sync char?

* Then we consume the invalid sequence into the invalid char
* And we start a new escape sequence in the buffer.

What do Claude say?

DARN! Claude has gone off the rails without me noting it.

* It has implemented full to_unicode(std::string_view,&UnicodeBytes)
* This iw WRONG for two reasons.
    1. I do NOT want out-parameters (Result should always come from function return value)
    2. I never intended Unicode to be used in buffering (only as intermediate code points) used by transcoding buffers.

I wonder if I can trust Claude to reactor this away? Or if the to_unicode architecture is already manifested in to much code?

I think I made Claude correct this error for now. At any case, I did what I can for now and want to proceed.

I now think I need a better harness in the generated code.

* I think I will ask Claude to add spdlog with my preferred facade LOG_BUSINESS, LOG_DEVELOPMENT_TRACE and LOG_DESIGN_INSUFFICIENCY.
* I think I will ask Claude to remember to NEVER use out parameters?
* I think I will ask Claude to NEVER implement code that is not explicitly required by implemented functionality or asked for by me?

I now vibe coded UTF8 to Unicode and made Claude configure for all-warnings-as-errors.

So now I have iterated to having 'open/ create project' based on uuid. 

* Seems to work just fine?
* Although allows for path with spaces (should use lowercase and '_' for CLI convenience?)

## 20261019

So this is take three to try and 'vibe code' a Swedish book keeping app in modern C++23.

This time I will try to 'drop feed' claude to get the code I can trust and want to maintain and use.

1. I provide a small task to Claude that shall produce a small output
2. I review, test run, tweak and commit the code and files
3. I update the input information that Claude can use for the next task
4. I repeat 1..4 until I have the app I want.

I started Claude but the sandboxing did NOT work.

* It quickly read the previous sessions for cratchit development.
* So now the context window is poisoned with presumptions from those session.

I will try to restart claude with NO permissions to read outside the sandbox!

This worked fine so far.

* I asked Claude for what files it proposed as 'context' during development.

```sh
.
├── CLAUDE.md
├── architecture.md
├── brief.md
├── conventions.md
├── decisions.md
├── domain.md
└── task.md
```

I now tried to edit these document. But this is already too complicated!

* The Claude.md is fine (the global genera guard rails about sandboxing and iterating small tasks based on listed md-files)
* The task.md is overkill. I enter the task in Claude Code as we go.

You know what. I chat with Claude to tweak the set of documents.

```sh
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/plaiground/session/60cae9fe % tree -L 1
.
├── CLAUDE.md
├── progress.md
└── session.md
```

MUCH BETTER!

So the first task is to create the scaffolding for a cross platform cmake based modern C++ application. It shall implement all functionality natively or consume from conan package manager when appropriate.

Claude asked me some clarifying questions and then created the scaffolding just fine.

```sh
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/plaiground/session/60cae9fe % tree
.
├── CLAUDE.md
├── cratchit
│   ├── CMakeLists.txt
│   ├── CMakeUserPresets.json
│   ├── README.md
│   ├── conanfile.py
│   ├── src
│   │   ├── core
│   │   │   ├── version.cpp
│   │   │   └── version.hpp
│   │   └── main.cpp
│   └── test
│       ├── CMakeLists.txt
│       └── test_version.cpp
├── progress.md
└── session.md

5 directories, 12 files
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/plaiground/session/60cae9fe % 
```