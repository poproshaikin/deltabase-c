# DeltaBase — Maturita Project Roadmap

This document outlines planned features and improvements for the one-year development period
leading up to the maturita defense. Items are grouped by visibility and ordered by priority.

---

## Current State (as of June 2026)

### What works
- Full SQL pipeline: lexer → parser → semantic analysis → planning → execution
- `SELECT`, `INSERT`, `UPDATE`, `DELETE`, `CREATE TABLE`, `ALTER TABLE ADD COLUMN`, `DROP TABLE`
- `CREATE SCHEMA`, `CREATE INDEX`, `DROP INDEX`
- `WHERE` with comparison operators (`=`, `!=`, `>`, `>=`, `<`, `<=`, `IS NULL`)
- `NOT NULL`, `DEFAULT`, `PRIMARY KEY` constraints
- B+ tree indexes (unique and non-unique)
- Buffer pool with LRU eviction
- Write-Ahead Log (WAL) with full REDO/UNDO/CLR recovery
- Implicit per-query transactions
- TCP server + .NET client protocol

---

## Group 1 — Visible SQL Features

These are user-facing features. Without them the database feels incomplete.

### `AUTO_INCREMENT` / `SERIAL` (Month 1–2)
The constraint struct already exists in the codebase. It needs to be enforced during `INSERT`:
read the current max value for the column, increment, assign. Required for primary keys to be
usable in practice.

### Explicit transactions — `BEGIN`, `COMMIT`, `ROLLBACK` (Month 2)
Currently every statement is wrapped in an implicit single-statement transaction. Explicit
transactions are needed to demonstrate ACID properties and to support realistic multi-statement
workloads. The `Transaction` class and WAL infrastructure are already in place — this is mostly
a matter of exposing the API through the parser and CLI.

### `ORDER BY` + `LIMIT` / `OFFSET` (Month 3)
Without `ORDER BY`, query results are non-deterministic and hard to test. Implementation:
after row materialization, sort by the specified columns using `std::sort` with a comparator
built from the column list. `LIMIT`/`OFFSET` are then trivially applied on top.

### Aggregate functions — `COUNT`, `SUM`, `AVG`, `MIN`, `MAX` (Month 3–4)
Requires a new `AggregateNode` plan node that accumulates values during a scan. A full
implementation also needs `GROUP BY` (partition rows by key before aggregating) and `HAVING`
(filter on aggregate results after grouping). This is one of the most fundamental relational
features and makes a strong impression at the defense.

### `INNER JOIN` and `LEFT JOIN` (Month 7–8)
The most complex visible feature. Requires:
1. Parser support for `JOIN ... ON` syntax
2. A join plan node in the planner
3. Nested-loop join as the baseline implementation

If time allows, a hash join can be added on top as an optimization — it reduces join complexity
from O(n·m) to O(n+m) for equality predicates and demonstrates query optimization thinking.

---

## Group 2 — Internal / Invisible Features

These are what separate a student project from a real database engine. They are harder to
explain in a demo but demonstrate a deep understanding of database internals.

### WAL checksums (Month 2)
Currently a corrupted WAL record is not detected — recovery silently replays garbage. Adding
a CRC32 checksum to each WAL record (~50 lines of code) makes the system genuinely crash-safe
rather than crash-safe only in theory. At the defense, this is easy to explain: "we detect
corruption and stop recovery rather than silently producing wrong results."

### Checkpoints (Month 5–6)
Currently the entire WAL is replayed from the beginning on every startup. On a long-running
instance this becomes catastrophically slow. A checkpoint writes a `BEGIN_CHECKPOINT` record
to the WAL, flushes all dirty pages to disk, then writes `END_CHECKPOINT`. On recovery, the
log only needs to be replayed from the last completed checkpoint.

This is a core topic from the ARIES recovery algorithm. Having checkpoints demonstrates that
the WAL implementation is production-grade, not just a classroom exercise.

### Cost-based query optimizer with basic statistics (Month 9–10)
Currently the planner always chooses a sequential scan regardless of whether an index exists.
A basic cost-based optimizer requires:
1. **Statistics collection:** store per-column stats (row count, number of distinct values,
   min/max). Updated on `INSERT`/`DELETE` or via an explicit `ANALYZE` command.
2. **Selectivity estimation:** estimate what fraction of rows a predicate will match using
   the statistics.
3. **Plan selection:** compare estimated cost of seq scan vs. index scan; choose the cheaper
   one.

Even a simple model (uniform distribution assumption) produces a meaningfully better planner
than always-seq-scan.

### Predicate pushdown (Month 10)
If a query is `SELECT * FROM a JOIN b WHERE a.id = 5`, the filter `a.id = 5` should be
applied before the join, not after. This is implemented as a plan tree rewriter: a pass that
walks the plan tree and moves filter nodes as close to their source scan as possible. The
performance difference is dramatic on large tables, and it demonstrates understanding of
algebraic query rewriting.

### Page-level locking (Month 11 — stretch goal)
Currently a single `recursive_mutex` per database instance means two concurrent `SELECT`
statements block each other. Replacing this with table-level latches in shared/exclusive modes
allows read parallelism and is a foundational concurrency topic. A basic implementation with
shared reads and exclusive writes is achievable; full two-phase locking (2PL) is more involved.

---

## Group 3 — Project Infrastructure

This is what makes the project look professional at the defense.

### `EXPLAIN` / `EXPLAIN ANALYZE` (Month 10)
A command that prints the query plan tree — which nodes were chosen, estimated row counts,
and (for `ANALYZE`) actual row counts observed at runtime. This makes the planner visible and
is essential for demonstrating the optimizer. Every production database has this command.

### Integration test suite (Month 11)
A proper test suite that creates tables, inserts data, runs queries, and asserts on results.
Even 50–100 targeted tests covering the main SQL features, constraint enforcement, and recovery
behavior would make the project significantly more credible. `Catch2` is a good lightweight
choice for C++.

### Benchmarks (Month 11)
Concrete performance numbers to show at the defense:
- Insert N rows; measure throughput
- Sequential scan vs. index scan on a large table; show the speedup
- With vs. without checkpoints; show recovery time difference

Numbers on a slide are far more convincing than "it's fast."

---

## Priority Schedule

| Period | Focus | Why |
|--------|-------|-----|
| Month 1–2 | Bug fixes, `AUTO_INCREMENT`, explicit transactions | Low-hanging fruit; makes the DB actually usable |
| Month 3–4 | `ORDER BY` + `LIMIT`, aggregate functions + `GROUP BY` | Visible results; good for early demos |
| Month 5–6 | Checkpoints | Strongest "invisible" feature; core DB theory topic |
| Month 7–8 | `INNER JOIN` (nested-loop), `LEFT JOIN` | Most expected feature of a relational DB |
| Month 9–10 | Cost-based optimizer + `EXPLAIN` | Demonstrates planning and optimization understanding |
| Month 11–12 | Tests, benchmarks, documentation, polish | Defense preparation |

Hash join and page-level locking are stretch goals — add them if time permits.

---

## What Makes This Strong as a Maturita Project

- **Breadth:** covers all major DB subsystems (parsing, planning, execution, storage,
  transactions, recovery)
- **Depth:** WAL + REDO/UNDO recovery and a cost-based optimizer are topics rarely seen even
  in university-level projects
- **Honesty:** bugs are documented, limitations are acknowledged, roadmap is realistic
- **Demonstrability:** CLI, benchmarks, and `EXPLAIN` output give concrete things to show
- **Modern C++:** C++20 features, clean architecture, modular design

---

*Last updated: June 2026*
