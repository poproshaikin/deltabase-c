# Dokumentace systemu DeltaBase

## Prehled

DeltaBase je vlastni databazovy system implementovany v jazycich C a C++. System se sklada z nekolika modulu, ktere spolu komunikuji a zajistuji spravu dat, jejich uklada­ni, vyhledavani a manipulaci.

## Architektura systemu

### 1. Jadro (Core)

- Implementovano v jazyce C.
- Bere vsechnu zodpovednost s praci s souborovym systemem.
- Nabizi zakladni API, ktere vyuziva Executor.

### 2. Moduly v C++

- #### 1. Sql
  - Modul, odpovidajici za parsovani dotazu
  - Lexicka analyza: rozbor dotazu na tokeny, vyhodnocovani typu kazdeho tokenu (napr. identifikator, literal, ...)
  - Syntakticka analyza: sestaveni abstraktniho syntaktickeho stromu na zaklade rozparsovanych tokenu

- #### 2. Executor
  - Semanticka analyza: kontrola existence tabulek, sloupcu, kompatibilita typu
  - Vyhodnocovani dotazu:
    - executor dostava zanalyzovany syntakticky strom a na jeho zaklade presmerovava dotazy do jadra
    - V budoucnosti: vytvorit planner a optimizer dotazu pro zrychleni exekuce, napr. smazani zbytecnych podminek (1 == 1)

### 3. Prubeh dotazu
  - Klient odesle SQL prikaz serveru (pres CLI nebo sit) -- v realizaci
  - Server preda dotaz modulu Sql pro parsovani
  - Sql parser provede lexickou a syntaktickou analyzu, vytvori AST
  - Executor provede semantickou analyzu, zkontroluje spravnost
  - Pripravi se optimalizovany plan exekuce -- v realizaci
  - Executor zavola nizkourovnove API pro praci s daty
  - Jadro nacte/zapise data se souboroveho systemu, aplikuje filtry
  - Vysledek se zformatuje a odesle se zpatky klientovi

### 4. Budouci rozsireni
  - Optimalizace dotazu (planner, optimizer)
  - Podpora transakci a rollback
  - TCP server
  - Indexovani
  - Rozsireni podporovanych datovych typu

### 5. Kompilace a build system
  - Jadro se kompiluje pomoci GCC
  - Moduly v C++ se kompiluji pomoci g++
  - Jako build system se pouziva CMake