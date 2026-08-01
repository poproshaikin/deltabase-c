# Storage Layer Refactor Plan

## Problem

`StdDbInstance` grew into a fat facade that absorbed executor responsibilities.
The executor became a thin pass-through with no real purpose.

## Goal

Split `StdDbInstance` into focused services. Executor becomes the orchestration layer.

---

## Services

### `StorageServiceProvider`
Owns shared singletons, constructs and holds all services.

Singletons it owns:
- `BufferPool`
- `CatalogCache`
- `IOManager`
- `WalManager`

Each service gets its dependencies injected via constructor.

---

### `DdlService`
Schema/table/index/sequence lifecycle.

Methods: `create_table`, `create_schema`, `drop_table`, `create_index`, `drop_index`, `create_sequence`

Note: does NOT auto-create PK indexes — that's the executor's job.

---

### `DmlService`
Raw data writes. No constraint logic, no preprocessing.

Methods: `insert_row`, `update_row`, `delete_rows`

Receives already-prepared and already-validated row.

---

### `DqlService`
All reads. Used by executors AND other services (e.g. ConstraintEnforcer).

Methods: `seq_scan`, `seq_scan_begin`, `seq_scan_next`, `index_scan`, point lookup by condition

---

### `RowPreprocessor`
Rewrites the row before validation/insert. Pure transformation, no IO.

Responsibilities:
- Fill autoincrement columns (increments sequence)
- Normalize column order (map named/positional cols to canonical schema order)

---

### `ConstraintEnforcer`
Validates the prepared row. Throws on violation.

Responsibilities:
- NOT NULL on PK columns
- UNIQUE / PRIMARY KEY (via index lookup)
- FK existence check (uses `DqlService` to find parent row)

Dependency: `DqlService` — one direction only, no cycle.

---

## Transaction Flow

```
Engine
  └─> TransactionManager.begin()  →  Transaction
        └─> passed as Transaction& into executors
              └─> passed as Transaction& into services
                    └─> txn.append_log(...)
```

`TransactionManager` used only at engine level for lifecycle (begin/commit/rollback).
Services only receive `Transaction&` — they never call `TransactionManager` directly.

---

## Dependency Graph

```
Executor
  ├─> DdlService
  ├─> DmlService
  ├─> DqlService
  └─> ConstraintEnforcer
        └─> DqlService

All services
  └─> BufferPool / CatalogCache / IOManager  (via constructor injection)
```

No cycles.

---

## Executor Examples

```cpp
// CreateTableNodeExecutor
auto mt = ddl.create_table(table_def);
for (auto& index : mt.indexes)
    ddl.create_index(index, mt.id);

// InsertNodeExecutor
auto effective_row = row_preprocessor.prepare_row(mt, cols, row, txn);
constraint_enforcer.validate_or_throw(mt, effective_row);
dml.insert_row(mt, effective_row, txn);
```

---

## What Happens to `StdDbInstance`

Removed or kept as a minimal shell only for initialization (`init()`, `~destructor`).
All method implementations move to respective services.
