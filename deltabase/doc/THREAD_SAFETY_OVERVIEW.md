# DeltaBase Thread Safety and Filesystem Concurrency Overview

## Scope
This document summarizes a code audit focused on thread safety, transaction concurrency, WAL durability ordering, and filesystem consistency before introducing a multi-threaded network server.

## Executive Summary
The current architecture is not ready for concurrent request processing with shared database state.

Main blockers:
- Transaction LSN assignment flow can create incorrect `prev_lsn` chains under concurrency.
- WAL/data flush ordering can violate write-ahead logging guarantees.
- Shared in-memory structures are mutated without synchronization.
- File updates are not atomic and can expose partial writes to readers.

## Critical Findings

### 1) LSN race in transaction flow
Transaction code calls `append_log(...)` and then reads `get_next_lsn() - 1` as the assigned LSN.

Under concurrent appends, another thread may increment `next_lsn_` between these calls, causing:
- Wrong `last_lsn_` inside a transaction.
- Broken `prev_lsn` chain.
- Recovery ambiguity.

### 2) WAL ordering risk during commit
`commit()` currently does:
1. Append commit record.
2. Flush WAL.
3. Flush all dirty pages.

Because dirty-page flushing is global, one transaction can flush pages produced by another transaction whose WAL records are not durable yet. This may violate WAL ordering.

### 3) Data races on shared state
There is no locking around key shared components:
- Buffer pool containers and table-page maps.
- Generic cache internals (`unordered_map` + replacement policy state).
- Catalog cache maps and lookups.
- Mutable database metadata and row/index mutations.

In C++, unsynchronized concurrent access to these structures is undefined behavior.

## High Priority Findings

### 4) Incorrect page selection in buffer pool
`prepare_dp(...)` scans all cached pages and can select a page from another table, instead of restricting selection to the target table.

This is a correctness bug even in single-threaded mode and becomes more dangerous with concurrency.

### 5) Missing isolation/concurrency control model
Current write operations mutate shared state directly without transaction-level locks/latches (no clear 2PL or MVCC discipline), enabling:
- Lost updates.
- Unique-check races.
- Inconsistent reads under concurrent writes.

### 6) Non-atomic metadata/page file overwrite
`write_file(...)` and `fsync_file(...)` overwrite target files directly (`truncate + write`).

Readers can observe torn/partial data, and crash consistency is weaker than an atomic replace pattern.

Recommended pattern:
- Write to temp file.
- `fsync(temp)`.
- `rename(temp, target)`.
- `fsync(parent directory)`.

## Medium Priority Findings

### 7) Engine object is not safe for shared multi-thread use
`Engine` reuses parser/planner/analyzer/database members as mutable shared state. Calling query execution concurrently on a shared `Engine` instance is unsafe.

### 8) WAL durability wait API mismatch
`FileWalManager` has `commit_wait(lsn)`, but this is not part of `IWALManager`. Transaction code cannot reliably use this through the interface.

## Recommended Implementation Roadmap

### Phase 1 (Must have before multi-threaded server)
1. Change WAL API so append returns the assigned LSN atomically.
2. Use returned LSN in transaction state (`last_lsn_`).
3. Introduce durable-LSN waiting in interface (for example `wait_for_durable(lsn)`).
4. Enforce page flush rule: do not write a page with `page_lsn > durable_lsn`.
5. Fix `BufferPool::prepare_dp(...)` to select only pages of the target table.

### Phase 2 (Core synchronization)
1. Add synchronization to cache/buffer/catalog structures.
2. Introduce lock hierarchy:
- Schema-level lock for DDL.
- Table-level shared/exclusive lock.
- Page/index latches for fine-grained mutations.
3. Define and enforce one isolation policy (at minimum Read Committed).

### Phase 3 (Filesystem hardening)
1. Use atomic file replace for metadata/pages/index files.
2. Keep directory fsync semantics explicit for durability.
3. Add corruption-safe reads with validation and clear error handling.

## Additional Notes
- If multiple processes may access the same database, process-level file locking (for WAL and data paths) is required in addition to in-process mutexes.
- Recovery behavior should be re-verified after WAL/flush changes with crash tests.

## Suggested Validation Checklist
- Concurrent insert/update/delete stress tests with 4 to 32 worker threads.
- WAL chain integrity checks (`prev_lsn` continuity per transaction).
- Crash-recovery tests at random interruption points.
- Duplicate-key race tests for unique indexes.
- Filesystem fault injection for partial/failed writes.
