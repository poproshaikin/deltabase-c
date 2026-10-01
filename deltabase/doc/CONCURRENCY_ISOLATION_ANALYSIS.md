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
---

# Part II — Follow-up analysis (code audit)

## 8. The axis is not processes — it is storage stacks per session

`engine::Engine` owns a `std::unique_ptr<StorageServiceProvider>`
(`src/engine/include/engine.hpp:28`), and `NetServer` holds
`std::unordered_map<types::UUID, engine::Engine> sessions_`
(`src/network/include/server.hpp:24`) with a detached
`std::thread` per accepted connection (`src/network/server.cpp:442-448`).
`Engine::attach_db` → `reset_storage` → `std::make_unique<StorageServiceProvider>(cfg)`
(`src/engine/engine.cpp:52`).

So **N sessions attached to the same database = N complete, independent
storage stacks inside one server process**: N `FileWalManager` (each with
its own `next_lsn_`), N `CatalogCache` (each with its own `MetaTable`
copies and `last_rid`), N `BufferPool` (each with its own page copies),
N `TransactionManager`, N `CheckpointManager` background threads,
N `FlushCoordinator` background threads
(`src/storage/storage_service_provider.cpp:20-84`).

This reframes §5. Every defect in §3 reproduces **between two sessions of a
single server process**, with no `fork()` involved:

- `next_lsn_` is hydrated per `FileWalManager` instance at construction
  (`update_next_lsn` over the WAL files, `src/wal/file_wal_manager.cpp:120-129`),
  then incremented in process-local memory under `mtx_`
  (`append_log`, `:153`). Two instances in one process collide exactly as
  two processes do.
- `last_rid` is per `CatalogCache`, loaded from `.meta` at `hydrate()`.
  Same.
- `DatabaseIoLockService::shared()` being process-local (§3) is not the
  reason this breaks. It would not help even if it were cross-process: the
  counters are not shared state behind that lock, they are *duplicated*
  state in each stack.

**The real decision is therefore not "multi-process: yes/no". It is
"one storage stack per session, or one per attached database".** Assuming
"there is always exactly one server" does not make the problem go away.

## 9. Worse than the counters: `recover()` runs on every attach

`StorageServiceProvider`'s constructor calls
`recovery_manager_->recover()` unconditionally
(`src/storage/storage_service_provider.cpp:48-49`) — no clean-shutdown
flag, no control-file gate, no exclusivity check. `RecoveryManager::recover()`
reads the whole log (`wal_.read_all_logs()`, `src/recovery/recovery_manager.cpp:23`)
and UNDOes every transaction without a `COMMIT` record.

Consequence: opening a second session (or process) against a database that
has an **open transaction** in the first one makes the newcomer treat that
transaction as a recovery loser — it UNDOes the first session's in-flight
work and writes CLRs for it, while the first session continues to believe
the transaction is live. Attach is not a read-only operation; it is a
destructive one.

This, not the LSN collision, is the strongest argument in the whole
document: concurrent independent attachment is not merely *incomplete*
today, it is *actively destructive*, and no counter-allocation scheme
from §5 addresses it.

## 10. State of intra-process (inter-thread) protection, component by component

| Component | Protection present | Actual state |
|---|---|---|
| `BufferPool::mutex_` | map structure only | **unsafe** — hands out `DataPage*`; contents mutated outside the lock |
| page pinning | none | **unsafe** — eviction erases the map node, outstanding pointers dangle |
| `Cache::evict_one` | — | **deadlocks** (see 10.3) |
| `CatalogCache::mutex_` | map structure only | **unsafe** — hands out `MetaTable*`; counters mutated outside the lock |
| index path (`IndexFile`/`BPIndexPager`) | none at all | **unsafe** — whole-file RMW, no latch, no `flock` |
| `FileWalManager::mtx_` | own state | correct for its own state (see 10.6) |
| `TransactionManager::active_transactions_mutex_` | ATT map | correct |
| `Engine` | none | not thread-safe; fine only while sessions are 1:1 with threads (see 10.8) |
| `CheckpointManager` | `checkpoint_mtx_` | correct per instance, **wrong per database** (see 10.9) |

### 10.1 `BufferPool::mutex_` protects the map, not the pages

Every public accessor takes `mutex_`, calls its `_impl`, and returns a raw
`DataPage*`/`IndexFile*` into the cache entry — then releases the lock.
All actual mutation happens in the caller, unlocked:
`append_row` callers, `dirty_dp`, `BPIndexPager`, and the read path
`DQLService::seq_scan_next` which does
`auto* page = buffer_pool_.get_dp(cursor.page); ... page->rows[cursor.slot++]`
(`src/storage/dql_service.cpp:95-108`) — bounds-checked against a `rows.size()`
that another thread may change between the check and the index.

Meanwhile `FlushCoordinator`'s background thread (every 200 ms,
`src/storage/flush_coordinator.cpp:19-22`) reaches
`flush_dirty_impl`, which copies `entry.value` *under* `mutex_`
(`src/storage/buffer_pool.cpp:519-527`). Holding `mutex_` does not exclude
the writer, because the writer is not holding it — so the flusher can
serialize a half-mutated page. This is a live data race on every build,
not a theoretical one.

### 10.2 No pinning

`misc::Cache` has no pin/unpin and no refcount. `put` → `evict_one` →
`map_.erase(it)` (`src/misc/include/cache.hpp:82-96`) destroys the entry
while callers may still hold `DataPage*` into it. Reachable at
`max_size_ = 10000` pages (≈320 MB of one table).

### 10.3 Eviction self-deadlocks

`put_dp_impl`/`create_dp_impl` hold `BufferPool::mutex_` (a non-recursive
`std::mutex`) → `Cache::put` → `evict_one` → `victim.flush(victim)` →
`BufferPool::flush(DataPageBuffer::CacheEntry&)` → `std::lock_guard lock(mutex_)`
(`src/storage/buffer_pool.cpp:254-258`). The first eviction hangs the
process. Single-threaded bug, independent of any concurrency decision.

### 10.4 `CatalogCache` — same shape

`get_table_impl` returns `&it->second.value` (`src/storage/catalog.cpp:232-236`),
and `mt.last_rid++` (`MetaTable::make_row`), `mt.total_rows++`,
`mt.live_rows--` (`insert_row_locked`/`delete_row_locked`) all run with no
lock held, concurrently with `flush()`/checkpoint snapshots reading the same
fields. Pointer *validity* is safe (`std::unordered_map` nodes are stable),
the *values* are not.

### 10.5 The index path has no protection whatsoever

`types::IndexFile` is the entire index in one object
(`std::vector<IndexPage> pages`, `src/types/include/index_file.hpp:13-20`),
cached in `index_files_` and written wholesale by `io_.write(IndexFile)`.
So:
- Two writers do read-modify-write of the whole index → last writer wins →
  the other's index entries are silently lost. Data pages are protected by
  `PageFileLock`; index files are not protected by anything.
- `BPIndexPager::get_page` returns `IndexPage*` into `file->pages`
  (`src/storage/BP_index_pager.cpp:47-56`) and `create_page` `push_back`s
  into that same vector — invalidating every outstanding `IndexPage*`.
  Latent even single-threaded.
- `DMLService::check_row_constraints` (`src/storage/dml_service.cpp:36-67`)
  is a check-then-act over the index with no lock: two concurrent inserts of
  the same key both pass the uniqueness check. **UNIQUE/PRIMARY KEY is not
  enforceable under any concurrency today.**

### 10.6 WAL manager

`mtx_` correctly guards `next_lsn_`/`dirty_`/`flushed_`. Note that
`append_log(const std::vector<WALRecord>&)` loops over single-record
`append_log` (`src/wal/file_wal_manager.cpp:168-180`) and `DbGuard` is a
`recursive_mutex`, so the batch is *not* atomic against other appenders —
records from two transactions interleave inside what the caller thinks is
one batch. ARIES tolerates this (grouping is by `prev_lsn` chains, not
adjacency), but it is worth knowing before anything relies on batch
contiguity.

### 10.7 Transactions hold no locks at all

`txn::Transaction` has `id_`, `state_`, `last_lsn_` and nothing else — no
lock set, no read/write set, no lock manager anywhere in the tree. There is
no 2PL and no MVCC: `DataRow` carries only `OBSOLETE`
(`src/types/include/data_row.hpp:17-20`) — no `xmin`/`xmax`, no creating
txn id. Visibility is "is the row flagged obsolete", nothing more.

### 10.8 `Engine` / session identity

`Engine` is not thread-safe (`parser_`, `active_txn_`, `ctx_`) and does not
need to be — one connection, one thread, one `Engine`. But
`NetServer::get_session` returns `&sessions_.at(session_id)` under
`sessions_mutex_` and the pointer is used after the lock is released
(`src/network/server.cpp:20-29`), and `handle_close_message` erases the
entry. Two connections presenting the *same* session UUID put two threads
inside one `Engine` and can dangle that pointer. Worth an explicit
"one live connection per session id" check regardless of everything else.

### 10.9 Checkpointing is per-session, which makes checkpoints wrong

`CheckpointManager` is constructed and `start_background`ed per
`StorageServiceProvider` (`src/storage/storage_service_provider.cpp:52-59`).
With two sessions on one database:
- two independent threads append `BeginCkpt`/`EndCkpt` into the same WAL;
- both overwrite the same control file with their own `redo_lsn`
  (`io_.write_control_file`, `src/recovery/checkpoint_manager.cpp:99-100`);
- each snapshot is taken from *its own* `BufferPool::snapshot_dpt()` and
  `TransactionManager::snapshot_att()`, so every checkpoint omits the other
  session's active transactions and dirty pages.

A checkpoint that under-reports the DPT yields a `redo_lsn` that is too
high, so recovery starts REDO *after* a change it needed to replay. Silent
data loss on the next crash, caused purely by having two sessions open.

## 11. Durability ordering is violated (independent of concurrency)

`insert_row_locked` appends the `InsertRecord` via `txn.append_log(...)` —
which only pushes onto `FileWalManager::dirty_` — and then does
`io_.write(*page, true)`, i.e. **fsyncs the data page while its log record
is still only in memory** (`src/storage/buffer_pool.cpp:154-168`; same in
`delete_row_locked`, `:213-219`). There is no `ensure_durable(lsn)` between
them.

Crash in that window leaves a page whose `last_lsn` names a record that
does not exist on disk, holding a change from an uncommitted transaction
that can therefore never be undone. This is a WAL-protocol violation, and
it is load-bearing: the page fsync has to happen inside the `PageFileLock`
for the lock to mean anything cross-process, which is exactly what forces
the write to be eager. Fixing the ordering and dropping the cross-process
requirement are the same piece of work.

## 12. Closing §4: the verifier is not under-counting, the scan is

Root cause found, and it is a consequence of §8, not a separate mystery.

`BufferPool::prepare_dp_impl` links a newly created page to the current tail
only if the table already has pages *in this stack's*
`data_pages_per_table_` (`src/storage/buffer_pool.cpp:325-364`). Both
workers started on an empty table, so each created a page with no tail to
link to. The table ends up with **two independent page chains, each with its
own head and `next == null`**.

`DQLService::seq_scan_begin` picks one page that is not referenced as
anyone's `next` and returns it as the cursor start; `seq_scan_next` then
walks **only** `page->next` (`src/storage/dql_service.cpp:86-116`). One
chain, one page, 100 rows. `dp_dump` disagrees because it enumerates the
directory, as does `BufferPool::get_table_data` — which is why DML's
phase-1 candidate collection sees both pages while `SELECT` does not.

Two things to take from this:
1. It is a latent single-stack bug too: **any** page not reachable through
   the chain is invisible to `SELECT` but visible to DML and to recovery.
   The scan should iterate `data_pages_per_table_` (the directory-derived
   map) and use `next` only as an ordering hint, or page linking must be
   made an invariant that cannot be skipped.
2. §4 can be struck from the open-questions list.

## 13. Which isolation invariants DeltaBase holds today

Measured against the standard levels, for **two sessions** (threads or
processes — §8 makes them equivalent):

| Anomaly | Prevented? | Why |
|---|---|---|
| Dirty read | **no** | `insert_row_locked` fsyncs uncommitted rows immediately; `DataRow` has no txn id, so another stack reading the page from disk returns them |
| Dirty write / lost update | **no** | no locks; `put_dp_impl` after a disk re-read discards another path's unflushed in-memory changes |
| Non-repeatable read | no | no read locks, no snapshot |
| Phantom | no | as above |
| Lost index entry | **no** | §10.5 whole-file RMW |
| Duplicate unique key | **no** | §10.5 check-then-act |
| Atomicity of a rolled-back txn | **no** | its uncommitted rows were already visible and may already have been read |
| Globally unique LSN | **no** | §3.1 / §8 |
| Unique `RowId` per table | **no** | §3.2 / §8 |

So the honest statement is **below READ UNCOMMITTED**: the level ladder
assumes atomicity and unique identity hold, and here they do not.

For **one session** the engine is serial by construction, so SERIALIZABLE
holds trivially — modulo the single-threaded defects (§10.3 eviction
deadlock, §10.5 index pointer invalidation, §11 ordering, §12 scan
chain), and modulo the two background threads of that same stack, which
race the session thread per §10.1.

## 14. Recommendation

**Multi-process concurrent attachment: out of scope, and *enforced*, not
merely documented. Multi-session concurrency inside one server: in scope,
served by one storage stack per database rather than one per session.**

Rationale:
1. Nothing in §9–§11 is cheaper to solve cross-process than in-process. The
   counters of §3 are the smallest part of the problem; `recover()`-on-attach,
   the per-stack checkpointer, the duplicated catalog, and the unprotected
   index are all shared *mutable* state that would need a shared-memory or
   file-lock protocol each. All three candidate designs in §5(a-c) address
   2 of ~9 problems.
2. Shared-page-cache-across-processes is a research-grade problem. Every
   real engine that allows multiple writing processes (Oracle RAC, DB2
   pureScale) does it with a dedicated coherency layer; SQLite allows
   multiple processes precisely *because* it gives up a shared buffer pool
   and serializes writers with a file lock. Postgres uses one process group
   with shared memory, i.e. the moral equivalent of one stack.
3. The educational payload of this project — ARIES, 2PL, buffer management —
   is entirely in-process. Cross-process coherency would add plumbing, not
   DB-internals insight.

### 14.1 Work items, in dependency order

- **A. Exclusive attach, enforced.** A `<db>.lock` file held open for the
  lifetime of the attachment with `flock(LOCK_EX | LOCK_NB)`; on failure,
  fail the attach with "database is in use". Same primitive as
  `PageFileLock`, ~50 lines, and it is what makes `recover()`-on-attach
  correct (exactly one recoverer, at exclusive start). Do this first: it
  stops the destructive case in §9 immediately.
- **B. Cheap independent fixes:** §10.3 eviction deadlock, §11 WAL
  ordering (`ensure_durable(lsn)` before the page fsync; then the page
  write no longer needs to be eager), §12 scan over
  `data_pages_per_table_`, §10.8 one-connection-per-session check.
- **C. One `StorageServiceProvider` per attached database**, refcounted by
  normalized db path, shared by all sessions in the process; `Engine` keeps
  only per-session state (`parser_`, `active_txn_`, `ctx_`). This deletes
  §3.1, §3.2.2, §3.3 and §10.9 by construction rather than patching each —
  one `next_lsn_`, one catalog, one checkpointer, one flusher, one recovery
  pass per database.
- **D. Concurrency control: one rwlock per attached database, held for the
  whole transaction** (not per statement) — exclusive for any transaction
  that writes, shared for read-only transactions. This is degenerate
  table-level 2PL, and because there is exactly one lock there is no
  deadlock and no lock manager to build. It yields genuine SERIALIZABLE:
  no dirty reads, no lost updates, no phantoms, and uniqueness checks
  become sound. Cost: one writer at a time per database — the same deal
  SQLite ships.
- **E. Pinning + per-page latches in `BufferPool`.** Required for D to be
  real, because the flush and checkpoint threads of the shared stack keep
  running concurrently with the writer even under a global transaction
  lock. Minimum viable shape: `get_dp` returns an RAII guard that pins the
  entry and holds a per-page `shared_mutex`, instead of a raw pointer. Same
  treatment for `IndexFile` (§10.5), which also removes the
  whole-file-RMW loss. This fixes §10.1, §10.2 and §10.4 as a group.
- **F. Rewrite `run_concurrent_two_processes_test`** as a multi-threaded,
  single-process test: several sessions over one shared stack, asserting the
  §13 table now reads "prevented" for every row. Keep a small second test
  that asserts the *second* process fails to attach (item A).
- **G. `PageFileLock`:** keep the class, stop relying on it for
  correctness, and drop the in-`flock` fsync once §11 is fixed. Rationale
  for keeping it at all: cheap belt-and-braces if the lock file is ever
  bypassed (e.g. `dp_dump` run against a live database).

### 14.2 Explicitly out of scope (document, do not half-support)

- MVCC / snapshot isolation / non-blocking readers (would need
  `xmin`/`xmax` on `DataRow`, a version chain and a vacuum story).
- Row- or page-granular 2PL with a lock manager and deadlock detection.
- Multiple *writing* processes against one database.
- Replication, distributed transactions.

Each is a separate project. Naming them as out of scope is worth more than
the current situation, where the code implies support for the third one.
