# Thread Safety and I/O Changes Overview

This document summarizes the changes made to improve concurrency safety, filesystem coordination, and WAL ordering in DeltaBase.

## 1) WAL LSN Handling

- Changed WAL append APIs to return the assigned `LSN` directly.
- Removed the race where transaction code derived `last_lsn_` via `get_next_lsn() - 1`.
- `Transaction::begin()`, `append_log()`, and `commit()` now use the returned `LSN`.

### Files
- [src/wal/include/wal_manager.hpp](src/wal/include/wal_manager.hpp)
- [src/wal/include/file_wal_manager.hpp](src/wal/include/file_wal_manager.hpp)
- [src/wal/file_wal_manager.cpp](src/wal/file_wal_manager.cpp)
- [src/transactions/transaction.cpp](src/transactions/transaction.cpp)

## 2) Durable WAL Wait API

- Added `wait_for_durable(LSN)` to the WAL interface.
- Kept `commit_wait()` as a compatibility wrapper.
- `Transaction::commit()` now waits for the commit `LSN` to become durable.

### Files
- [src/wal/include/wal_manager.hpp](src/wal/include/wal_manager.hpp)
- [src/wal/include/file_wal_manager.hpp](src/wal/include/file_wal_manager.hpp)
- [src/wal/file_wal_manager.cpp](src/wal/file_wal_manager.cpp)
- [src/transactions/transaction.cpp](src/transactions/transaction.cpp)

## 3) Write-Ahead Guard for Pages and Index Files

- Added `flush_dirty(max_lsn)` to the buffer pool.
- Data pages are only flushed if `page.last_lsn <= max_lsn`.
- Added `last_lsn` to `IndexFile` and serialized/deserialized it.
- Index files now participate in the same WAL ordering guard.

### Files
- [src/storage/include/buffer_pool.hpp](src/storage/include/buffer_pool.hpp)
- [src/storage/buffer_pool.cpp](src/storage/buffer_pool.cpp)
- [src/types/include/index_file.hpp](src/types/include/index_file.hpp)
- [src/storage/std_binary_serializer.cpp](src/storage/std_binary_serializer.cpp)
- [src/storage/std_db_instance.cpp](src/storage/std_db_instance.cpp)

## 4) Buffer Pool Correctness Fix

- Fixed `BufferPool::prepare_dp()` so it only scans pages that belong to the target table.
- Removed the bug where a page from another table could be reused.

### Files
- [src/storage/buffer_pool.cpp](src/storage/buffer_pool.cpp)

## 5) Index Mutation LSN Tracking

- Added `set_if_lsn(index_id, lsn)` to the buffer pool.
- `insert_row_into_indexes()` now returns touched index IDs.
- Index files are stamped with the latest relevant `LSN` after mutations.
- Unique index creation also stamps the index file with the transaction `LSN`.

### Files
- [src/storage/include/buffer_pool.hpp](src/storage/include/buffer_pool.hpp)
- [src/storage/buffer_pool.cpp](src/storage/buffer_pool.cpp)
- [src/storage/include/db_instance.hpp](src/storage/include/db_instance.hpp)
- [src/storage/include/std_db_instance.hpp](src/storage/include/std_db_instance.hpp)
- [src/storage/include/detached_db_instance.hpp](src/storage/include/detached_db_instance.hpp)
- [src/storage/detached_db_instance.cpp](src/storage/detached_db_instance.cpp)
- [src/storage/std_db_instance.cpp](src/storage/std_db_instance.cpp)

## 6) Coarse-Grained Instance Synchronization

- Added a `std::recursive_mutex` to `StdDbInstance`.
- Guarded public DB operations to serialize access to shared in-memory state.
- This reduces data races for one shared engine instance.

### Files
- [src/storage/include/std_db_instance.hpp](src/storage/include/std_db_instance.hpp)
- [src/storage/std_db_instance.cpp](src/storage/std_db_instance.cpp)

## 7) Shared DB-Scoped I/O Lock Service

- Added a shared `DatabaseIoLockService` that returns one recursive mutex per database identity.
- Wired the service through I/O and WAL factories.
- `FileIOManager`, `DetachedFileIOManager`, and `FileWalManager` now share the same db-scoped lock inside the process.
- This prevents concurrent filesystem conflicts when multiple engines point to the same DB path.

### Files
- [src/storage/include/db_io_lock_service.hpp](src/storage/include/db_io_lock_service.hpp)
- [src/storage/include/io_manager_factory.hpp](src/storage/include/io_manager_factory.hpp)
- [src/wal/include/wal_manager_factory.hpp](src/wal/include/wal_manager_factory.hpp)
- [src/storage/include/file_io_manager.hpp](src/storage/include/file_io_manager.hpp)
- [src/storage/include/detached_file_io_manager.hpp](src/storage/include/detached_file_io_manager.hpp)
- [src/wal/include/file_wal_manager.hpp](src/wal/include/file_wal_manager.hpp)
- [src/storage/file_io_manager.cpp](src/storage/file_io_manager.cpp)
- [src/storage/detached_file_io_manager.cpp](src/storage/detached_file_io_manager.cpp)
- [src/wal/file_wal_manager.cpp](src/wal/file_wal_manager.cpp)

## 8) Validation

All changes were validated with a successful build:

```bash
cmake --build . -j4
```

## Notes

- The current solution is process-local. It protects multiple threads and multiple engines within the same process.
- It does not yet protect multiple processes writing to the same database directory.
- A future step could add atomic file replacement and OS-level file locks for cross-process safety.
