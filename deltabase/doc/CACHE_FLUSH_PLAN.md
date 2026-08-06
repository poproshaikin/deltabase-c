# Cache Flush Standardization & Async WAL/Page Flush — Plan

## 1. Problem statement

Cache/page writes to disk currently happen from many uncoordinated places, each
with its own fsync policy. Worse, `Transaction::commit()` performs the WAL
fsync **and** the buffer-pool page/index flush **synchronously, on the
committing thread, per statement** (every autocommit DML is its own
transaction — see `Engine::execute_query`, `src/engine/engine.cpp:141-149`).
That is why a single DML statement currently costs a minimum of several
fsyncs (~12ms observed) when it could cost one WAL fsync (~1ms) with the page
cache flushed lazily in the background.

Goal: introduce one component responsible for *all* disk writes of cached
state (data pages, index files, catalog/meta objects), move it off the
transaction-commit critical path, and only keep the WAL fsync as the
synchronous durability point (which is the only thing the WAL protocol
actually requires).

## 2. Current state — inventory of every flush/fsync site

### 2.1 Buffer pool (data pages + index files)
- `BufferPool::flush(DataPageBuffer::CacheEntry&)` — `src/storage/buffer_pool.cpp:16` →
  `io_.write_page(page, /*fsync=*/true)`.
- `BufferPool::flush(IndexFileBuffer::CacheEntry&)` — `buffer_pool.cpp:25` →
  `io_.write_index_file(file, /*fsync=*/true)`.
- `BufferPool::flush_dirty()` (unconditional, flushes **every** dirty entry) —
  called from `~BufferPool()` (`buffer_pool.hpp:61`, process/db shutdown) and
  from `BPIndexPager::flush()` (`BP_index_pager.cpp:89`), which itself has
  **no callers** anywhere in the codebase today (dead code path).
- `BufferPool::flush_dirty(LSN max_lsn)` — flushes only pages/indexes whose
  `last_lsn <= max_lsn`. **Called from exactly one place:**
  `Transaction::commit()` (`src/transactions/transaction.cpp:74`), synchronously,
  right after the WAL fsync, on every single commit. This iterates the *entire*
  cache map (`data_pages_ | std::views::values`, `index_files_ | ...`) checking
  `dirty` + LSN per entry — O(cache size) per commit, on top of one
  `fsync_file()` syscall sequence per dirty entry found.
- `misc::Cache::evict_one()` (`src/misc/include/cache.hpp:86`) — when the LRU
  cache hits `max_size_` (hardcoded `100`, `cache.hpp:25`) during `put()`, it
  synchronously flushes the evicted entry if dirty. This is a **hidden fsync
  buried inside an insert call**, can fire from any code path that creates a
  new page/index entry once the cache is full.

### 2.2 Catalog (schemas / tables / sequences)
- `CatalogCache::flush()` (`src/storage/catalog.cpp:45`) unconditionally
  rewrites **every** schema/table/sequence with individual
  `io_.write_ms/write_mt/write_seq(..., fsync=true)` calls. **Only called from
  `~CatalogCache()`** (`catalog.cpp:15`), i.e. on clean process shutdown.
  There is no dirty-tracking for catalog objects at all (unlike `BufferPool`,
  which at least has per-entry `dirty` flags) and no incremental/commit-time
  flush — metadata durability today relies entirely on WAL replay after a
  crash, and the on-disk catalog files are just a shutdown-time snapshot.
- `RowPreprocessor::process_row` (`src/storage/row_preprocessor.cpp:84,98`)
  calls `io_manager_.write_seq(*seq)` **directly**, bypassing `CatalogCache`
  and `BufferPool` entirely, on every row insert that touches an
  `AUTOINCREMENT` column — one physical file write per row, regardless of
  whether the surrounding transaction commits or rolls back (the sequence
  file write is not undone on rollback; only the WAL record is compensated).
- `DetachedDbInstance`/`DetachedFileIOManager` — N/A, these throw
  `logic_error` for all write paths (used only for `CREATE DATABASE`
  bootstrap, not part of the flush pipeline).
- **Duplicated rollback mechanism.** `CatalogCache` maintains its own
  in-memory undo log, `CatalogDelta` (`catalog.hpp:15-27`, `txn_deltas_`),
  hand-populated by every `put`/`save_table`/`save_schema`/`delete_table`/
  `delete_schema`/`delete_sequence` call, consumed only by
  `CatalogCache::rollback_txn()` (`catalog.cpp:103-121`). Meanwhile
  `RecoveryManager` already has a **second, independent** WAL-driven undo
  mechanism for the exact same record types —
  `undo_record(const CreateSchemaRecord&)` / `UpdateSchemaRecord` /
  `CreateTableRecord` / `UpdateTableRecord` / `CreateIndexRecord` /
  `DropIndexRecord` / `CreateSequenceRecord` / `UpdateSequenceRecord`
  (`recovery_manager.cpp:477-537`) — but it's only ever called from
  `RecoveryManager::undo()` during crash recovery. `Transaction::rollback()`'s
  WAL-walk (`transaction.cpp:93-136`) never calls these for catalog records;
  every catalog record type falls into its generic `else { current =
  r.prev_lsn; }` branch. Compare this to pages, where there is only **one**
  undo mechanism (`RecoveryManager::undo_record(InsertRecord&, DataPage&)` /
  `UpdateRecord` / `DeleteRecord`), shared by both live rollback and crash
  recovery. See §4.3 for the proposed fix.

### 2.3 WAL
- `FileWalManager::append_log()` — cheap, just pushes to an in-memory
  `dirty_` vector under `mtx_` (`file_wal_manager.cpp:135`). No I/O.
- `FileWalManager::flush()` (`file_wal_manager.cpp:235`) → `write_logs()` →
  opens the current WAL segment (`O_APPEND`), writes records, `fsync(fd)` +
  `close(fd)` (`file_wal_manager.cpp:292-343`). If a batch spans a WAL segment
  boundary (every `MAX_RECORDS_PER_LOGFILE = 1000` records), it fsyncs+closes
  the old segment and opens a new one mid-batch.
- `FileWalManager::wait_for_durable(lsn)` (`file_wal_manager.cpp:194`) already
  implements a reasonable **group-commit** pattern: if a flush is already in
  progress, concurrent callers just wait on a condition variable instead of
  each doing their own fsync. This part is *not* the bottleneck by itself —
  the bottleneck is that it's invoked synchronously from `Transaction::commit`
  and `Transaction::rollback`, and is immediately followed by a second,
  unrelated synchronous flush (`BufferPool::flush_dirty`).
- `RecoveryManager` calls `wal_.flush()` directly at a few points during
  recovery (`recovery_manager.cpp:396,418,434`) — acceptable, recovery is a
  single-threaded startup-only phase.

### 2.4 Recovery (startup-only, bypasses caches by design)
`RecoveryManager` writes pages/schemas/tables/sequences straight to disk via
`io_.write_page/write_ms/write_mt/write_seq` during the REDO/UNDO passes
(`recovery_manager.cpp:97,123-253,400,485-537`), without going through
`BufferPool`/`CatalogCache` at all, since those caches aren't populated yet at
that point in startup. This is legitimate and should remain a documented
exception, not be forced through the new coordinator.

### 2.5 Transaction commit/rollback (the hot path)
`Transaction::commit()` (`src/transactions/transaction.cpp:65-77`):
```
append_log(commit_record)          // cheap, in-memory
wal_manager_->wait_for_durable(lsn) // fsync #1 (WAL) — required by WAL protocol
buffer_pool_->flush_dirty(lsn)      // fsync #2..N (one per dirty page/index!) — NOT required synchronously
catalog_->commit_txn(id_)           // just erases the txn delta, no I/O
```
`Transaction::rollback()` also calls `wait_for_durable` (line 143) for the
rollback record, which is correct (rollback record must be durable before
telling the caller the rollback succeeded), but does **not** call
`flush_dirty` — asymmetry that already hints the page flush doesn't actually
need to be on the commit path.

### 2.6 Checkpointing — currently a no-op
`types::Config::last_checkpoint_lsn` (`src/types/include/config.hpp:95`) is
read by `RecoveryManager` to skip already-applied records
(`recovery_manager.cpp:42,51,67`) but **is never written/advanced anywhere in
the codebase**. It is always `0`. This means every crash recovery today
replays the *entire* WAL history from LSN 1, and the WAL directory grows
forever with no truncation. This is directly relevant here: the flush
coordinator is the natural owner of checkpoint advancement, since a
checkpoint is only valid once we know which pages are durably flushed.

## 3. Why this causes ~12ms per DML

For an autocommit `INSERT` into a table with a primary key (auto-created
B+Tree index):
1. `Engine::execute_query` sees no active txn → begins an implicit txn, runs
   the insert, calls `commit_active_txn()` (`engine.cpp:130-149,166`).
2. `Transaction::commit()` → `wait_for_durable` → 1 fsync (WAL segment).
3. `Transaction::commit()` → `flush_dirty(lsn)` → iterates full cache, finds
   the dirty data page → `fsync_file()` (open+write+fsync+close) → finds the
   dirty PK index file → another `fsync_file()`.

Three synchronous fsyncs, sequentially, on the same thread, per statement.
Steps 2 and 3b/3c are independent pieces of state — the WAL entry is already
the durable source of truth for both the page and the index change (that's
the entire point of write-ahead logging: **REDO from WAL can always
reconstruct a page**, so the page/index file only needs to be flushed
*before that WAL segment could be needed for recovery / gets truncated*, not
at commit time).

## 4. Proposed design

### 4.1 New component: `storage::FlushCoordinator`
Single owner of **all** physical writes of cached state:
`DataPage`, `IndexFile`, `MetaSchema`, `MetaTable`, `MetaSequence`.
Lives in `src/storage/`, constructed by `StorageServiceProvider` alongside
`BufferPool`/`CatalogCache`, and takes references to both plus `IIOManager`
and `IWALManager`.

Responsibilities:
- Maintains one dirty queue (page/index/catalog-object id + its LSN),
  populated by `BufferPool::dirty_dp/dirty_if` and by a new
  `CatalogCache::mark_dirty(...)`-style API (see 4.3), instead of each cache
  scanning its own map on every commit.
- Exposes `flush_up_to(LSN wal_durable_lsn)` — flush only entries whose
  `last_lsn <= wal_durable_lsn` (preserves the WAL-before-page invariant:
  never write a page to disk before the WAL record that produced it is
  durable).
- Exposes `flush_all()` for graceful shutdown (replaces today's
  `BufferPool::~BufferPool` / `CatalogCache::~CatalogCache` implicit
  destructor flushes — those become explicit calls from
  `StorageServiceProvider::~StorageServiceProvider` instead of relying on
  member-destruction order).
- Owns a background thread (or is driven by `TransactionManager`) that wakes
  up periodically (e.g. every 50–200ms) or when the dirty queue crosses a
  size/byte threshold, and calls `flush_up_to(wal.get_durable_lsn())`. This
  is the *only* place `fsync_file()` gets called for pages/indexes/catalog
  objects going forward (excluding the documented recovery exception, §4.7).
- Advances `Config::last_checkpoint_lsn` once a flush pass completes, and
  persists it via the existing `io_manager_->write_cfg(cfg_)` path — this
  finally makes the dead `last_checkpoint_lsn` field do something, bounding
  future recovery replay time and enabling WAL segment truncation later.

### 4.2 New fast commit path
`Transaction::commit()` becomes:
```
append_log(commit_record)
wal_manager_->wait_for_durable(lsn)   // still the only synchronous fsync
catalog_->commit_txn(id_)             // unchanged, in-memory only
state_ = COMMITTED
```
The `buffer_pool_->flush_dirty(last_lsn_)` call is removed from the commit
path entirely. Dirty pages/indexes touched by this transaction stay in the
cache (already correctly ordered by LSN) and get picked up by
`FlushCoordinator`'s background pass. Durability is preserved because REDO
recovery reconstructs any page from the WAL if the process crashes before the
background flush runs.

`Transaction::rollback()` is unaffected (no page flush happened there
before either).

### 4.3 CatalogCache: unify rollback via WAL-driven undo, remove `CatalogDelta`
Bring catalog undo in line with how pages already work instead of leaving two
parallel mechanisms that must be kept in sync by hand (see §2.2):
- Add `RecoveryManager::undo_record(const XxxRecord&, CatalogCache&)`
  overloads (or place them on `CatalogCache` itself, e.g.
  `CatalogCache::apply_undo(const XxxRecord&)`) for every catalog record type
  that currently has a disk-writing `undo_record` overload
  (`CreateSchemaRecord`, `UpdateSchemaRecord`, `DeleteSchemaRecord`,
  `CreateTableRecord`, `UpdateTableRecord`, `DeleteTableRecord`,
  `CreateIndexRecord`, `DropIndexRecord`, `CreateSequenceRecord`,
  `UpdateSequenceRecord`). These new overloads patch `tables_`/`schemas_`/
  `sequences_` **in memory only** — no I/O — using the same before-image
  fields the existing disk-writing overloads already read. This is exactly
  the relationship `undo_record(InsertRecord&, DataPage&)` already has to a
  page: patch the in-memory object, don't touch disk.
- Extend `Transaction::rollback()`'s `visit` (`transaction.cpp:93-136`) to
  match these catalog record types and call the new in-memory overload,
  exactly like it already does for `InsertRecord`/`UpdateRecord`/
  `DeleteRecord`.
- Delete `CatalogDelta` and `txn_deltas_` entirely (`catalog.hpp:15-27,35`),
  along with every `push_back` scattered through `put`/`put_or_update`/
  `save_table`/`save_schema`/`delete_table`/`delete_schema`/
  `delete_sequence`. `CatalogCache::rollback_txn()` goes away — undo now
  happens in `Transaction::rollback()`'s WAL walk, same as pages.
- The existing **disk-writing** `RecoveryManager::undo_record(const
  XxxRecord&)` overloads are untouched — they keep serving crash recovery
  (§4.7) exactly as today.

### 4.4 CatalogCache: real dirty tracking for flush (separate from rollback)
Once §4.3 removes `CatalogDelta`, dirty-tracking for the *flush* path is a
clean, independent addition instead of being layered on top of rollback
bookkeeping. Add it directly at the point `tables_`/`schemas_`/`sequences_`
are mutated (`put`/`save_table`/`save_schema`/`delete_*`), tagged with the
WAL LSN of the record that caused the change:
- `FlushCoordinator` flushes only dirty catalog objects whose LSN is durable,
  same as pages.
- `CatalogCache::flush()` keeps only the "flush everything, unconditionally"
  behavior, used solely by the shutdown path (§4.8).

### 4.5 RowPreprocessor stops writing sequences directly
`io_manager_.write_seq(*seq)` calls in `row_preprocessor.cpp:84,98` are
replaced with `catalog_.mark_dirty(seq->id, wal_lsn)` (or equivalent) —
sequence state flows through the same cache + coordinator as everything else
instead of being physically written once per row. This also fixes the
existing inconsistency where a rolled-back transaction still leaves the
sequence file physically advanced on disk (which §4.3 also fixes from the
rollback side, since sequences gain a real in-memory undo path too).

### 4.6 `misc::Cache::evict_one()`
Eviction under memory pressure should not silently call a synchronous
flusher inline inside `put()`. Options to resolve during implementation
(pick one, not both):
- (a) increase `max_size_` / make it configurable and rely on the background
  coordinator to keep the dirty set small enough that eviction of *dirty*
  entries is rare, or
- (b) have `evict_one()` hand the dirty victim to `FlushCoordinator`'s queue
  and flush it out-of-line instead of inline.
This needs a decision before implementation; recommend (a) first since
`max_size_ = 100` is almost certainly too small and is likely the actual
reason eviction-triggered flushes happen at all today.

### 4.7 Recovery stays a documented exception
`RecoveryManager`'s disk-writing `redo`/`undo_record` overloads keep writing
directly via `IIOManager` during REDO/UNDO — it runs before
`BufferPool`/`CatalogCache` are hydrated, so there is nothing to route
through the coordinator yet. Add a comment at the top of
`recovery_manager.cpp` stating this explicitly so it isn't "fixed" into
using caches that don't exist yet by a future refactor. This is now a
slightly bigger surface than before §4.3 (the same record types get both a
disk-writing overload for crash recovery and a memory-writing overload for
live rollback) — worth a short comment pointing each pair at the other so
they aren't mistaken for duplicates to be merged.

### 4.8 Shutdown path
`StorageServiceProvider::~StorageServiceProvider()` (`storage_service_provider.cpp:73`)
currently only writes the config file and implicitly relies on
`buffer_pool_`/`catalog_` unique_ptr destructors to flush. Make this
explicit and ordered:
```
flush_coordinator_->stop_background_thread();
flush_coordinator_->flush_all();     // pages, indexes, catalog objects
wal_manager_->sync();                // drain any remaining WAL dirty_ buffer
io_manager_->write_cfg(cfg_);        // now includes an up-to-date last_checkpoint_lsn
```

## 5. Migration steps

1. Add `FlushCoordinator` with `flush_up_to`/`flush_all`, no background
   thread yet; wire it in place of the direct `flush_dirty(lsn)` call in
   `Transaction::commit()` (still synchronous, but from one place) — proves
   out the dirty-queue design without changing timing behavior yet.
2. Add dirty-queue tracking to `BufferPool` (avoid full-map scans) —
   `dirty_dp`/`dirty_if` push into the coordinator's queue instead of (or in
   addition to) the per-entry `dirty` bool.
3. Unify catalog rollback (§4.3): add the in-memory
   `undo_record(const XxxRecord&, CatalogCache&)` overloads, wire them into
   `Transaction::rollback()`'s WAL walk, then delete `CatalogDelta`/
   `txn_deltas_` and `CatalogCache::rollback_txn()`. Ship and test this on
   its own — it's a behavioral change to rollback correctness, independent
   of the flush-coordinator timing changes, and easier to verify in
   isolation (see §6 for the new test scenarios needed).
4. Give `CatalogCache` per-object dirty tracking + LSNs for the *flush* path
   (§4.4), now a clean addition on top of step 3 rather than layered on the
   old delta; remove the "rewrite everything" behavior from the hot path,
   keep it only for `flush_all()`.
5. Route `RowPreprocessor` sequence updates through `CatalogCache` instead of
   `IIOManager` directly.
6. Remove `buffer_pool_->flush_dirty(last_lsn_)` from `Transaction::commit()`;
   confirm via the existing WAL/recovery integration tests that killing the
   process immediately after a commit (before any background flush) still
   recovers correctly via REDO.
7. Add the background flush thread + threshold/interval triggers to
   `FlushCoordinator`.
8. Wire checkpoint advancement (`Config::last_checkpoint_lsn`) into the
   coordinator's flush pass and persist it via `write_cfg`.
9. Update `StorageServiceProvider` shutdown sequence per §4.8.
10. Decide and implement the `evict_one()` fix from §4.6.
11. (Optional follow-up, not required for this plan) WAL segment truncation
    once a checkpoint proves older segments are no longer needed for
    recovery.

## 6. Risks / things to verify

- **Crash-consistency regression risk** is the main one: removing the
  synchronous page flush from commit is only safe because REDO recovery must
  be able to fully reconstruct any committed-but-not-yet-flushed page from
  WAL. This already needs to be true for correctness today (a crash between
  WAL fsync and page fsync is already possible in the current code), so the
  change shouldn't introduce a *new* class of bug, but it makes that window
  much larger and much more likely to be hit in practice — the existing
  recovery integration tests (mentioned in recent commits, "add integration
  tests") should be extended with a "kill after commit, before background
  flush" scenario.
- `BufferPool::is_row_obsolete` and other in-memory readers already read
  from the cache regardless of `dirty`, so read-your-own-writes within the
  same process is unaffected by deferring the physical flush.
- Background thread lifecycle needs to cleanly stop before
  `IWALManager`/`IIOManager` are destroyed (destruction order in
  `StorageServiceProvider`).
- `misc::Cache` is a template used for both `DataPageBuffer` and
  `IndexFileBuffer` — any change to `evict_one()` affects both.
- **Catalog undo unification (§4.3) needs its own correctness pass**,
  separate from the flush-timing risk above: add rollback tests for
  `BEGIN; CREATE TABLE ...; ROLLBACK` (table must disappear from
  `CatalogCache`), `BEGIN; ALTER TABLE ...; ROLLBACK` (before-image
  restored), `BEGIN; CREATE/DROP SCHEMA ...; ROLLBACK`, `BEGIN; CREATE
  INDEX ...; ROLLBACK`, and an autoincrement insert followed by `ROLLBACK`
  (sequence value must revert). These previously worked via `CatalogDelta`;
  after §4.3 they must keep working via the new
  `undo_record(&, CatalogCache&)` path instead — regressing any of them
  would be a correctness bug, not just a performance one.

## 7. Files affected

- `src/storage/include/buffer_pool.hpp`, `buffer_pool.cpp`
- `src/storage/include/catalog.hpp`, `catalog.cpp` (§4.3 removes
  `CatalogDelta`/`txn_deltas_`/`rollback_txn`; §4.4 adds dirty tracking)
- `src/recovery/include/recovery_manager.hpp`, `recovery_manager.cpp` (§4.3
  adds the new `CatalogCache`-targeting `undo_record` overloads alongside
  the existing disk-writing ones)
- `src/transactions/transaction.cpp` (§4.3 extends the `rollback()` `visit`
  to cover catalog record types)
- `src/storage/row_preprocessor.cpp`
- `src/misc/include/cache.hpp`
- `src/storage/storage_service_provider.cpp`, `storage_service_provider.hpp`
- `src/wal/file_wal_manager.cpp` (only if checkpoint-driven WAL truncation is
  added; not required for the core fix)
- New: `src/storage/flush_coordinator.{hpp,cpp}`

## 8. Separate follow-up (not part of this plan): WAL segment preallocation

Independent of moving page/catalog flush off the commit path, the **WAL
fsync itself** can likely be made cheaper at the filesystem level. This does
not change any durability semantics (WAL fsync stays synchronous, as
established in §3/§4.2) — it only reduces its cost.

### 8.1 Current behavior
`FileWalManager::write_logs()` (`file_wal_manager.cpp:292-343`) opens each
WAL segment with `O_WRONLY | O_CREAT | O_APPEND` and lets the file grow
incrementally, one `write()` at a time, across possibly many `flush()` calls
over the segment's lifetime. The fd is opened and closed on *every single*
`flush()` call — segments are never kept open across flushes.

### 8.2 Why this is more expensive than necessary
Every write that **extends** a file's size requires the filesystem to
allocate new blocks/extents and commit that metadata change (on journaling
filesystems like ext4/xfs, this is an extra journal transaction + write
barrier on top of the data itself). A write that lands **inside a region the
file already occupies** does not need any of that — `fsync()` only has to
persist already-written data blocks, no metadata/journal commit involved.
Postgres avoids the expensive path by preallocating each 16MB WAL segment
fully (zero-filled) at creation time (`XLogFileInit`) and then only ever
overwriting already-allocated blocks for the lifetime of that segment — this
is very likely why one of the observed local Postgres commits landed at
~1.3ms while most others matched the disk's raw fsync floor (~12ms): it hit
an already-preallocated, already-"warm" segment, while the others coincided
with segment/file allocation work.

### 8.3 Proposed change
- Preallocate each new WAL segment file to its full target size at creation
  time (`posix_fallocate`/`ftruncate` + explicit zero-fill, mirroring
  Postgres) instead of letting it grow via `O_APPEND`.
- Write records via `pwrite()` at explicitly tracked offsets within the
  preallocated region instead of relying on `O_APPEND`.
- Keep the fd for the *active* segment open across flushes (open once when a
  segment becomes current, close only on rollover to the next segment)
  instead of open+close on every `flush()` call.

### 8.4 Design wrinkle to resolve before implementing
Postgres segments are a fixed byte size because WAL there is made of
fixed-size (8KB) pages. DeltaBase's `WALRecord`s are **variable-length**
(`record_size` is written as an explicit `uint64_t` prefix before each
serialized record — `file_wal_manager.cpp:329-335`), and segment rollover is
currently driven by a **record count** (`MAX_RECORDS_PER_LOGFILE = 1000`),
not a byte size. A fixed-size preallocated segment therefore needs one of:
- (a) preallocate to a generous estimated byte size and roll over to the
  next segment early (before hitting 1000 records) if the running byte
  offset would exceed the preallocated region, or
- (b) switch segment boundaries to be purely byte-size-based instead of
  record-count-based (simpler invariant, closer to how Postgres does it, but
  changes the WAL segment file naming scheme which currently encodes
  first/last LSN under the count-based assumption — recovery's directory
  scan/sort logic in `hydrate_cache()` would need to keep working with
  whatever the new naming scheme is).

This should be scoped and implemented as its own change, independently
testable against raw fsync latency (e.g. via `strace -T` before/after) before
being combined with the FlushCoordinator work in §4-§5.
