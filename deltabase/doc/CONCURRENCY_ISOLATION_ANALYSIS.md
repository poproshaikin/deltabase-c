# Concurrency & Isolation — Analysis

## 1. Problem statement

`page-level-locking` (this branch) added `PageFileLock` — an `flock()`-based,
per-page lock protecting data page *content* against concurrent writers,
plus `BufferPool::insert_row_locked`/`delete_row_locked`/retry-on-full-page
logic in `DMLService`. That work answers one narrow question: "can two
writers safely append/obsolete rows on the same physical page file." It does
**not** answer the much larger question this document is for:

- What are the actual concurrency/isolation invariants DeltaBase needs to
  hold (within one process, across threads; and, separately, across
  multiple OS processes attached to the same on-disk database)?
- Is true multi-process concurrent write access to one database a real
  target for this project, or should DeltaBase document/enforce
  "single attaching process at a time" and treat anything beyond that as
  out of scope?

That second question is not rhetorical — §3 below shows the codebase
currently *behaves* as if multi-process access is supported (there's a
two-process integration test for it, `tests/main.cpp`), but the actual
machinery backing that assumption is incomplete in a way that doesn't fail
loudly, it silently corrupts sequence numbers.

## 2. What is already cross-process safe (validated)

Running the existing concurrency test (`run_concurrent_two_processes_test`
in `tests/main.cpp`, forks two real OS processes, each attaching its own
`engine::Engine` to the same fresh database and inserting 100 rows
concurrently into the same table) and inspecting the result with
`dp_dump`/`wal_dump`/`mt_dump` confirms:

- **Page content writes are durable and uncorrupted across processes.**
  Both workers' 100 rows each landed on disk, on two distinct pages, with
  correct column data (`dp_dump --db <name> --schema common --table
  test_concurrent` shows 200 total rows, each with the right `(id, payload)`
  values). No torn writes, no lost rows, no deadlock between the two
  `PageFileLock` holders.

This is exactly the guarantee `insert_row_locked`/`delete_row_locked`
(`src/storage/buffer_pool.cpp`) were built to provide, and it holds.

## 3. Key finding: no counter in this codebase is cross-process safe

`DatabaseIoLockService::shared()` (`src/storage/include/db_io_lock_service.hpp:33-38`)
is a function-local `static` singleton holding a registry of
`std::recursive_mutex`. A `std::mutex`/`std::recursive_mutex` is a
**process-local** primitive — it does not synchronize across OS processes,
and (relevant for `tests/main.cpp`, which uses `fork()`) a mutex's
underlying OS state is not meaningfully shared across a `fork()` either once
the child starts running independently. Both `FileWalManager::next_lsn_`
(`src/wal/file_wal_manager.cpp`) and `types::MetaTable::last_rid`
(`src/types/meta_table.cpp`) are plain in-memory counters incremented only
under this process-local lock (or no lock at all, in `last_rid`'s case,
beyond whatever `CatalogCache`'s mutex already provides for that process).

### 3.1 Observed: 100% LSN collision between the two worker processes

`wal_dump concurrency_test_<pid>_<ts>` on the test's output database shows
every LSN from the point both workers start writing onward duplicated
exactly once — one occurrence per process:

```
[7] BEGIN_TXN txn=...62d9-626c... prev=0
[7] BEGIN_TXN txn=...62d8-6d76... prev=0
[8] INSERT txn=...62d9-626c... ... after=row{..., tokens=[{0}, {"worker_0"}]}
[8] INSERT txn=...62d8-6d76... ... after=row{..., tokens=[{100000}, {"worker_100000"}]}
```

`awk '{print $1}' | sort | uniq -c` over the full dump confirms this holds
for every LSN after record 6 (the point the two workers' writes start
interleaving) — a 100% collision rate, not an occasional race.

This is more fundamental than it first looks: ARIES-style recovery assumes
LSN is a single, strictly totally-ordered, globally unique sequence.
REDO idempotency (`page.last_lsn >= record.lsn` ⇒ skip), `prev_lsn` UNDO
chains, and checkpoint `last_checkpoint_end_lsn` semantics are all defined
in terms of that assumption. With two independently-counting processes,
that assumption is false today for any database attached by more than one
process at a time.

### 3.2 Observed: `last_rid` collides the same way (compounded by a second, unrelated bug)

Every row inserted by both workers in the test has `DataRow.id == 0`
(`dp_dump` output). Two separate causes stack here:

1. **Unrelated to cross-process concerns:** `InsertNodeExecutor`,
   `UpdateNodeExecutor`, and `DeleteNodeExecutor`
   (`src/executor/include/node_executor.hpp:191,246,279`) each declare
   `types::MetaTable mt_;` **by value**, not by reference, even though the
   planner (`src/executor/node_executor.cpp:767`,
   `const MetaTable& mt = *ssp.ddl().get_table(...);`) hands them a
   reference into the live, process-wide `CatalogCache` entry
   (`CatalogCache::get_table_impl` returns `&it->second.value`,
   `src/storage/catalog.cpp:232-236` — a pointer with session lifetime).
   Each node executor is one-shot per statement; copying `mt` into a
   by-value member means every mutation (`last_rid++` in
   `MetaTable::make_row`, `total_rows++`/`live_rows++` in
   `insert_row_locked`) is thrown away when that executor is destroyed.
   The *next* statement in the *same process* fetches the same
   never-updated cache entry again. This alone is sufficient to explain
   `id == 0` on every row from a single process, with zero multi-process
   involvement. **This should be fixed regardless of anything else in this
   document** — change the three fields to `types::MetaTable& mt_;`.

2. **The actual cross-process issue:** even after fixing (1), `last_rid`
   is `CatalogCache` state, loaded once per process from the table's
   `.meta` file at attach time, mutated only in that process's memory.
   Two processes attaching around the same time both start counting from
   the same base value with no shared sequence — exactly the same shape of
   bug as §3.1, just for row identity instead of WAL position. In this
   specific test it didn't manifest as duplicate `(page_id, row_id)` pairs
   because both workers started on an empty table and each created its own
   page on first insert (`data_pages_per_table_` is populated once at
   `BufferPool::initialize_impl()`, before either worker had written
   anything, so neither saw the other's page as a candidate). On a
   non-empty table where `prepare_dp` could hand both processes the *same*
   existing page, two processes would durably write rows with colliding
   `RowId` onto the same page — which breaks anything that looks a row up
   by `row_id` within a page (`BufferPool::find_row_owner`,
   `delete_row_locked`'s linear `for (auto& row : page->rows) if (row.id
   == row_id)`, `is_row_obsolete_impl`) ambiguously, returning the first
   match rather than the intended row.

## 3.3 Addendum: fixing the by-value `mt_` bug surfaces a third symptom

After fixing §3.2.1 (`InsertNodeExecutor`/`UpdateNodeExecutor`/`DeleteNodeExecutor`
now hold `types::MetaTable& mt_;`, `src/executor/include/node_executor.hpp:191,245,278`,
with the corresponding `from_plan` call sites in `src/executor/node_executor.cpp`
updated to bind non-`const` references), rerunning the same test produced a
*different* failure on the same database before even reaching the row-count
assertion:

```
FileIOManager::read_tables_meta: failed to deserialize
  .../test_concurrent/test_concurrent.meta
At least one worker failed during concurrent insert
```

Inspecting the same file after the run (via `mt_dump`) found it perfectly
valid — `last_rid=0 total_rows=0 live_rows=0`, i.e. whatever the table looked
like at creation time, before either worker's inserts. So this wasn't a
permanently corrupted file; it was a **transient read/write race** during the
run: most likely both workers' `~CatalogCache()` (process-exit catalog flush,
see `doc/CACHE_FLUSH_PLAN.md` §2.2 — `CatalogCache::flush()` is only called
from the destructor, on clean process shutdown) raced to rewrite the same
`.meta` file at close to the same time, and something in that sequence let a
reader observe a half-written or momentarily-missing file despite `write_mt`
(`src/storage/file_io_manager.cpp:481-490`) going through `fsync_file`'s
`flock(LOCK_EX)` and `read_file` taking `flock(LOCK_SH)` before reading. Not
root-caused beyond this; worth folding into the same investigation as §3,
since it's the same underlying gap (no process coordinates catalog-file
writes with each other) showing up as a crash/parse-error instead of silent
data loss this time.

## 4. Open, not yet explained: verifier only sees 100 of 200 physically-written rows

The test's final assertion (`select * from common.test_concurrent` after
both workers exit) returns 100 rows, not 200, even though §2 confirms both
workers' 200 rows are physically present and well-formed on disk at that
point (confirmed independently via `dp_dump`, a separate process reading
straight off disk). The verifier (`engine::Engine verifier;` in
`tests/main.cpp`) is a third, fresh `Engine`/`BufferPool` instance created
*after* both workers have exited, so this isn't an in-flight race — something
in how it resolves "which pages belong to this table" or how it executes the
scan is dropping one page's worth of rows. Not yet root-caused; needs its own
investigation pass before it can be ruled in or out of scope for this
document's recommendations. (`FileIOManager::map_data_pages_for_table`,
`src/storage/file_io_manager.cpp:592-636`, does do a real
`fs::directory_iterator` scan rather than trusting any cached list, so the
cause is likely downstream of that, not in it — but this hasn't been traced
further yet.)

## 5. The decision this analysis is blocked on

Everything above funnels into one scoping question that needs an answer
before implementation work continues:

**Is concurrent multi-process attachment to the same on-disk database a
real, supported scenario for DeltaBase — or should the project document and
enforce single-attaching-process-at-a-time, and treat
`run_concurrent_two_processes_test` as testing a scenario that's out of
scope?**

This matters because the two answers lead to very different amounts of
remaining work:

- **If multi-process is in scope:** `next_lsn_` and `last_rid` (and
  probably `total_rows`/`live_rows`, and anything else `CatalogCache`
  treats as process-local authoritative state) need a real cross-process
  allocation mechanism. Candidate designs, in increasing implementation
  complexity / decreasing per-call overhead:
  - **(a) `flock`-guarded counter file** — same pattern as `PageFileLock`:
    open a small file, `flock(LOCK_EX)`, read-increment-write, unlock.
    Simple, reuses an already-proven primitive. For `last_rid` (once per
    row insert) this is a small addition on top of the page-level flock
    that insert already pays. For `next_lsn_` (once per *any* WAL record,
    i.e. the hottest path in the system) a syscall round-trip per
    allocation is a real throughput concern.
  - **(b) `mmap`'d shared counter, atomic increment** — `MAP_SHARED` +
    `std::atomic_ref`/compiler intrinsics directly on the mapped memory.
    No syscall per allocation (CPU-level atomic fetch-add, visible across
    processes via normal cache coherency), at the cost of more delicate
    setup/initialization code and some platform-specific handling.
  - **(c) Range leasing** — a process reserves a batch of N ids/LSNs under
    one `flock` call, then hands them out from memory until exhausted
    before leasing another batch. Fewer syscalls than (a), simpler than
    (b); accepts gaps in the sequence (already fine for both LSN and
    RowId — neither needs to be dense, only unique and monotonic).
  - Whichever is chosen, it should be designed once and reused for both
    counters rather than solved twice.
  - Additionally: a full audit of what other per-process in-memory state
    `CatalogCache`/`BufferPool` treat as authoritative (not just these two
    counters) is needed before claiming multi-process correctness anywhere
    beyond "page bytes don't tear."
- **If single-process is the actual target:** the `flock`-based
  `PageFileLock` is solving a problem one level more general than
  necessary — an in-process `std::mutex`/`std::shared_mutex` per `page_id`
  would be simpler and cheaper, and `run_concurrent_two_processes_test`
  should be rewritten as a multi-*threaded*, single-process test instead
  (a materially smaller, more tractable correctness problem than true
  cross-process coordination).

## 6. Items to fix regardless of the decision in §5

1. ~~`InsertNodeExecutor`/`UpdateNodeExecutor`/`DeleteNodeExecutor` holding
   `MetaTable` by value instead of by reference (§3.2.1)~~ — **fixed** on
   this branch (now `types::MetaTable&`). Fixing it surfaced §3.3.
2. The verifier under-counting rows (§4) — needs root-causing before it can
   be filed as "expected given single-process scope" or "another
   cross-process bug."
3. The transient catalog-file read/write race on process exit (§3.3) — needs
   root-causing; likely the same missing cross-process coordination as §3,
   applied to `.meta` files instead of counters.

## 7. How to reproduce

```
cd build && ninja test
./build/bin/test
```

The test creates and cleans up its own fresh database per run
(`concurrency_test_<pid>_<timestamp>` under `build/bin/data/`); on failure
it leaves that database directory in place for inspection. Useful follow-up
commands against a leftover failed run (`<db>` = the printed db name):

```
./build/bin/mt_dump  --db <db> --schema common --table test_concurrent
./build/bin/dp_dump  --db <db> --schema common --table test_concurrent
./build/bin/wal_dump <db>
```