# DeltaBase

A relational database engine written from scratch in C++20.  
DeltaBase implements the full classic DB stack: SQL parsing, semantic analysis, query planning, Volcano-model execution, file-backed page storage, an LRU buffer pool, B+ tree indexes, sequences, multi-statement transactions, Write-Ahead Logging, and ARIES-style crash recovery.

---

## Features

- **SQL** — `SELECT`, `INSERT`, `UPDATE`, `DELETE`, `CREATE/DROP TABLE`, `ALTER TABLE`, `CREATE/DROP INDEX`, `CREATE DATABASE`, `CREATE SCHEMA`
- **Data types** — `INTEGER`, `REAL`, `CHAR`, `BOOL`, `TEXT`
- **Constraints** — `NOT NULL`, `UNIQUE`, `DEFAULT`, `AUTOINCREMENT` (sequence-backed), `PRIMARY KEY` (auto-creates B+ tree index)
- **Transactions** — explicit `BEGIN` / `COMMIT` / `ROLLBACK`; UUID-keyed transaction IDs
- **WAL** — append-only log with before/after images for data and DDL operations
- **Crash recovery** — ARIES REDO+UNDO with Compensation Log Records (CLR); no lost committed work, no partial writes
- **B+ tree indexes** — disk-resident, paged, used automatically for primary key lookups
- **Buffer pool** — LRU-evicting cache for data pages and index files; dirty-page tracking per transaction
- **`information_schema`** — virtual `tables` view exposing catalog metadata
- **TCP server** — remote connections with a session-per-UUID model and chunked result streaming
- **Interactive CLI** — linenoise-powered REPL with meta-commands (e.g. `\connect`, `\list`)
- **Diagnostic tools** — standalone dump utilities for WAL, data pages, table metadata, and index files

---

## Architecture

```
                    ┌──────────────────────────────────────────┐
                    │                  Engine                   │
                    │  parse → analyze → plan → execute         │
                    └──────────────┬───────────────────────────┘
                                   │
          ┌────────────────────────┼────────────────────────┐
          │                        │                        │
     ┌────▼────┐            ┌──────▼──────┐         ┌──────▼──────┐
     │   SQL   │            │  Executor   │         │   Storage   │
     │ Lexer + │            │  Semantic   │         │   Service   │
     │ Parser  │            │  Analyzer   │         │  Provider   │
     └─────────┘            │  Planner    │         └──────┬──────┘
                            │  Node       │                │
                            │  Executors  │    ┌───────────┼───────────┐
                            └─────────────┘    │           │           │
                                          ┌────▼───┐ ┌─────▼───┐ ┌────▼────┐
                                          │ Buffer │ │Catalog  │ │DDL/DML/ │
                                          │  Pool  │ │ Cache   │ │  DQL    │
                                          └────┬───┘ └─────────┘ └─────────┘
                                               │
                               ┌───────────────┼───────────────┐
                               │               │               │
                          ┌────▼────┐   ┌──────▼─────┐  ┌─────▼──────┐
                          │   WAL   │   │  IO Manager│  │  Recovery  │
                          │ Manager │   │  (files)   │  │  Manager   │
                          └─────────┘   └────────────┘  └────────────┘
```

### Module breakdown

| Module         | Path              | Responsibility                                                   |
|----------------|-------------------|------------------------------------------------------------------|
| `types`        | `src/types/`      | Shared data structures: `DataPage`, `DataRow`, `DataToken`, `WALRecord`, AST nodes, `Config`, `MetaTable/Column/Index/Schema` |
| `misc`         | `src/misc/`       | Utilities: LRU cache, `MemoryStream`, logger, exceptions, `convert` helpers |
| `sql`          | `src/sql/`        | Hand-written lexer (`Lexer`) and recursive-descent parser (`SqlParser`) producing an AST |
| `executor`     | `src/executor/`   | `SemanticAnalyzer`, `StdPlanner`, `NodeExecutor` (Volcano model: open / next / close), `Evaluator`, `InformationSchemaProvider` |
| `storage`      | `src/storage/`    | `StorageServiceProvider` (DI container), `BufferPool`, `CatalogCache`, `DDLService`, `DMLService`, `DqlService`, `FileIOManager`, `IndexBPlusTree` / `BPIndexPager`, `RowPreprocessor`, `ConstraintEnforcer` |
| `transactions` | `src/transactions/` | `Transaction` (begin/commit/rollback + WAL append), `TransactionManager` |
| `wal`          | `src/wal/`        | `IWALManager`, `FileWALManager`, `StdWALSerializer`              |
| `recovery`     | `src/recovery/`   | `RecoveryManager`: full REDO pass then UNDO pass with CLR generation |
| `engine`       | `src/engine/`     | `Engine` — top-level façade coordinating all layers              |
| `network`      | `src/network/`    | `NetServer`, `INetProtocol` / `StdProtocol`, `Session`, `SocketHandle` |
| `cli`          | `src/cli/`        | REPL (`Cli`), `MetaExecutor` (meta-commands), `ResultFormatter`  |
| `binaries`     | `src/binaries/`   | `main()` entry points for each executable                        |

---

## Storage layout

Data is stored relative to the executable's working directory:

```
data/
  <database>/
    <database>.meta          — database metadata
    <schema>/
      common.meta            — schema metadata
      tables/
        <table>/
          <table>.meta       — column and index definitions
          data/              — data pages (UUID-named files, up to 32 kB each)
          index/             — B+ tree index files
      sequences/
        <sequence>           — current sequence value
    wal/
      1_1000                 — WAL segment (LSN range encoded in filename)
```

### Data page format

- **Max size:** 32 kB
- **Header:** table UUID + page UUID + next-page UUID + min/max RowId + row count + last LSN
- **Body:** variable-length serialized `DataRow` records

---

## WAL & Recovery

Every data modification (INSERT/UPDATE/DELETE) and every DDL operation writes a WAL record **before** touching the page. Records carry:
- `lsn` — monotonically increasing Log Sequence Number (`uint64_t`)
- `prev_lsn` — previous LSN for the same transaction (chain)
- `txn_id` — UUID of the owning transaction
- Before/after images sufficient for REDO and UNDO

On crash, `RecoveryManager::recover()` runs:
1. **Analysis** — scan WAL; build sets of committed, rolled-back, and active (loser) transactions
2. **REDO** — replay all committed records and existing CLR records in LSN order
3. **UNDO** — walk losers' chains backward; for each undoable record, apply the inverse operation and write a CLR to the log

---

## Query execution pipeline

```
SQL string
   │
   ▼  SqlParser::parse()
  AST
   │
   ▼  SemanticAnalyzer::analyze()
  AnalysisResult  (validated + type-checked)
   │
   ▼  StdPlanner::plan()
  QueryPlan  (tree of plan nodes)
   │
   ▼  NodeExecutor  (Volcano: open / next / close)
  DataTable  (result rows)
```

---

## Building

**Requirements:**
- GCC 12+ (C++20)
- CMake 3.15+
- Ninja
- `libuuid` (`sudo dnf install libuuid-devel` / `sudo apt install uuid-dev`)

```bash
cd deltabase
cmake -G Ninja -B build .
cd build
ninja
```

Executables appear in `build/build/bin/`.

---

## Executables

| Binary           | Description                                                  |
|------------------|--------------------------------------------------------------|
| `cli.exe`        | Interactive REPL — connect to a database and run SQL queries |
| `server.exe`     | TCP server; accepts remote sessions over the custom protocol |
| `test.exe`       | Test suite                                                   |
| `wal_dump.exe`   | Print WAL log records for a database to stdout               |
| `dp_dump.exe`    | Hex/row dump of a data page file                             |
| `mt_dump.exe`    | Print table metadata (columns, indexes, constraints)         |
| `index_dump.exe` | Print B+ tree index file contents                            |

---

## Network protocol

`server.exe` listens on a configurable TCP port. Each client gets an independent `Engine` instance keyed by a session UUID. The custom binary protocol supports:

| Message type    | Purpose                                     |
|----------------|---------------------------------------------|
| `QueryMessage`  | Execute a SQL string; stream result chunks  |
| `CreateDbMessage` | Create a new database                    |
| `AttachDbMessage` | Attach (open) an existing database        |
| `CloseMessage`  | Close the session                           |

---

## information_schema

```sql
SELECT * FROM information_schema.tables;
```

Returns: `table_catalog`, `table_schema`, `table_name`, `table_type`, `column_count`, `index_count`, `total_rows`, `live_rows`.

---

## Design notes

- **No third-party DB library is used.** Every layer (parser, planner, storage, WAL, recovery, B+ tree) is written from scratch.
- **`StorageServiceProvider`** is the DI container for the storage layer; it owns infrastructure objects (IOManager, WALManager, BufferPool, CatalogCache, RecoveryManager, TransactionManager) and service objects (DDLService, DMLService, DqlService). Infrastructure is initialized first (declaration order matters).
- **Volcano model:** every plan node implements `open() / next() / close()`. `next()` returns one row at a time; the caller drives iteration.
- **LRU buffer pool** tracks dirty pages per transaction. On commit, dirty pages are flushed; on rollback, dirty pages are discarded from the pool.
- **Sequences** back `AUTOINCREMENT` columns; sequence state is persisted to disk and WAL-logged.
