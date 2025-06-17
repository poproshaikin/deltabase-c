# Dokumentace systému DeltaBase

## Přehled

DeltaBase je vlastní databázový systém implementovaný v jazycích C a C++. Systém se skládá z několika modulů, které spolu komunikují a zajišťují správu dat, jejich ukládání, vyhledávání a manipulaci.

## Architektura systému

### 1. Jádro (Core)

- Implementováno v jazyce C.
- Převzalo veškerou zodpovědnost za práci se souborovým systémem.
- Nabízí základní API, které využívá Executor.

### 2. Moduly v C++

#### 1. Sql

- Modul odpovídající za parsování dotazů.
- **Lexikální analýza**: rozbor dotazu na tokeny, vyhodnocování typu každého tokenu (např. identifikátor, literál, …).
- **Syntaktická analýza**: sestavení abstraktního syntaktického stromu na základě rozparsovaných tokenů.

#### 2. Executor

- **Sémantická analýza**: kontrola existence tabulek, sloupců, kompatibilita typů.
- **Vyhodnocování dotazů**:
  - Executor dostává zanalyzovaný syntaktický strom a na jeho základě přesměrovává dotazy do jádra.
  - *Do budoucna*: vytvořit planner a optimizer dotazů pro zrychlení exekuce, např. odstranění zbytečných podmínek (`1 == 1`).

### 3. Průběh dotazu

- Klient odešle SQL příkaz serveru (přes CLI nebo síť) – *v realizaci*.
- Server předá dotaz modulu Sql pro parsování.
- Sql parser provede lexikální a syntaktickou analýzu, vytvoří AST.
- Executor provede sémantickou analýzu, zkontroluje správnost.
- Připraví se optimalizovaný plán exekuce – *v realizaci*.
- Executor zavolá nízkoúrovňové API pro práci s daty.
- Jádro načte/zapíše data ze souborového systému, aplikuje filtry.
- Výsledek se naformátuje a odešle zpět klientovi.

### 4. Budoucí rozšíření

- Optimalizace dotazů (planner, optimizer)
- Podpora transakcí a rollbacku
- TCP server
- Indexování
- Rozšíření podporovaných datových typů
