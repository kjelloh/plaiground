# Consider vibe coded book keeping app cratchit take 1?

```#62c6984e```

## 20261016

So I want to see what I can create by vibe coding my book keeping app 'cratchit'?

I have four overall tasks I need cratchit to be able to do.

1. It must be able to produce a Swedish 'VAT Returns report' (Momsrapport) to send to the Swedish Tax Agency (SKV)
    * Also see "Todo: Consider the file format to send for a VAT Returns (Momsrapport) to Swedish Tax Agency?"
2. It must be able to 'close the books' (Bokslut) for a financial year
    * Also see all my notes "Todo: Eget AB - Överväg att dokumentera hur jag gjorde Bokslut för ..."
3. It must be able to produce the swedish document 'Årsrapport'
    * Also see all my notes "Todo: Eget AB - Överväg att dokumentera hur jag gjorde Årsredovisning för ..."
4. It must be able to produce a Swedish 'Tax Returns report' (Deklaration INK2) to send to the swedish tax agency
    * Also see "Eget AB: Överväg att dokumentera hur jag deklarerade för året 20240501 - 20250430"

I want to start with the first task to make a to-the-point and simple book keeping app that can produce a VAT REturns report to send to SKV.

* I want it to be able to import an SIE file with the current state of the books.
* I wan it to be able to process bank or SKV account statements to generate BAS account postings.
* I want it to be able to process vouchers and/or listings of other relevant financial events
* I want it to use existing entries in the SIE file to infer how new entries shall be created.
* I want it to be able to determine what financial events that has occurred and how to generate book entries with the correct and appropriate BAS account postings.
* I want it to be able to export any new state of the books as a new SIE file.