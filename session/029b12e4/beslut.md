# Beslut

Beslut som gäller i den här sessionen. Beslut från tidigare sessioner gäller inte här förrän de står i den här filen. Källorna att hämta dem från står i [status.md](status.md#källor).

## 2026-11-15

* Sessionen utgår från koden i 0a37da4b (`cratchit/`, git-spårade filer, steg 77). Den är kopierad hit oförändrad.
* `beslut.md` och `status.md` börjar om från början. Användaren och Claude tar in tidigare beslut och status en i taget, när arbetet behöver dem.
* Målet: en cratchit som går att använda på riktigt. De verifierade delarna från andra sessioner (årsredovisning, INK2, momsrapport, PDF till text) ska bli en del av den.
* cratchit installeras för hela datorn, på cmakes standardplats för plattformen (`/usr/local` på macOS och Linux, `C:\Program Files\cratchit` på Windows). Användarens skäl: ingen delar sin arbetsdator i dag. Användarens egna filer (`~/.cratchit`, samlingsmappen) är fortfarande per användare.
* `install.py` är borttaget för nu. Installationen sker med cmake (se `cratchit/README.md`).
