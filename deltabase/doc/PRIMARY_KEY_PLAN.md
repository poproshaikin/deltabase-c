# Primary Key — Implementation Plan

## Current State

- `MetaPrimaryKeyConstraint` already defined in `ColumnConstraint` variant — type exists, does nothing
- `MetaIndex::is_unique` + `insert_row_into_indexes` already enforces uniqueness — PK reuses this path
- Parser throws `UNSUPPORTED_STATEMENT` on PRIMARY KEY — the only blocked step
- Semantic analyzer checks NOT NULL on INSERT — just need to ensure PK column carries it

---

## Step 1 — `MetaIndex`: add `is_primary` flag

**File:** `src/types/include/meta_table.hpp`

```cpp
struct MetaIndex {
    // ...
    bool is_unique;
    bool is_primary = false;
};
```

A PK index is just a unique index with a special flag. No new structure needed.

---

## Step 2 — Parser: enable PRIMARY KEY

**File:** `src/sql/parser.cpp` — `parse_constraint()`

Remove `throw EngineException("Primary keys are not supported yet", ...)`, return `PrimaryKeyConstraint()`.

```cpp
if (kw == SqlKeyword::PRIMARY) {
    advance_or_throw("Expected KEY after PRIMARY");
    if (key->get_detail<SqlKeyword>() != SqlKeyword::KEY)
        throw EngineException("Expected KEY after PRIMARY", Code::SYNTAX_ERROR);
    advance();
    return PrimaryKeyConstraint();
}
```

---

## Step 3 — Semantic analyzer: validate CREATE TABLE

**File:** `src/executor/semantic_analyzer.cpp` — `analyze_create_table()`

- At most one PK per table
- PK column type must be orderable (`INTEGER`, `REAL`, `TEXT`, `CHAR`) — not `BOOL`
- PK must not be combined with `DEFAULT NULL`

```cpp
int pk_count = 0;
for (const auto& col_def : stmt.columns)
    for (const auto& c : col_def.constraints)
        if (std::holds_alternative<PrimaryKeyConstraint>(c)) pk_count++;

if (pk_count > 1)
    return AnalysisResult(EngineException(
        "Table can have at most one primary key", Code::SYNTAX_ERROR));
```

> Compound keys (`PRIMARY KEY (a, b)`) are out of scope for this plan.

---

## Step 4 — `create_table`: auto-create index and NOT NULL

**File:** `src/storage/std_db_instance.cpp` — `create_table()`

After saving the table to the catalog, find the PK column and:

1. Inject `MetaNotNullConstraint` if not already present
2. Create a `MetaIndex` with `is_unique = true`, `is_primary = true` via the existing index creation path

```cpp
for (auto& col : mt.columns) {
    if (!col.has_constraint<MetaPrimaryKeyConstraint>()) continue;

    if (!col.has_constraint<MetaNotNullConstraint>())
        col.constraints.push_back(MetaNotNullConstraint{});

    MetaIndex pk_index;
    pk_index.name      = "_pk_" + mt.name;
    pk_index.column_id = col.id;
    pk_index.key_type  = col.type;
    pk_index.is_unique = true;
    pk_index.is_primary = true;
    // allocate root page via buffer pool, add to mt.indexes
}
```

`insert_row_into_indexes` will automatically pick up this index on every INSERT.

---

## Step 5 — INSERT: NULL guard for PK columns

**File:** `src/storage/std_db_instance.cpp` — `insert_row()`

Current behavior: the index skips NULL values (does not insert into B+ tree) — so duplicate NULL rows are not caught.

Add an explicit check **before** `insert_row_into_indexes`:

```cpp
for (const auto& idx : mt->indexes) {
    if (!idx.is_primary) continue;
    int64_t col_pos = mt->get_column_idx(idx.column_id);
    if (row.tokens[col_pos].type == DataType::_NULL)
        throw EngineException(
            "PRIMARY KEY column cannot be NULL", Code::NOT_NULL_VIOLATION);
}
```

---

## Step 6 — Serialization: persist `is_primary`

**File:** `src/storage/std_storage_serializer.cpp`

Add `is_primary` next to `is_unique` in the `MetaIndex` serialization block.

> **Note:** this is a breaking format change. Existing `.db` files must be recreated or a
> version migration added to the deserializer.

---

## Step 7 — `information_schema.tables`: expose PK column

**File:** `src/executor/information_schema_provider.cpp`

In `get_virtual_table()` add a `primary_key_column` output column.
In `get_virtual_data()` find the index with `is_primary = true` and return the column name,
or NULL if the table has no PK.

---

## Implementation Order

| # | File | Effort |
|---|------|--------|
| 1 | `meta_table.hpp` — add `is_primary` | trivial |
| 2 | `parser.cpp` — enable PRIMARY KEY | easy |
| 3 | `semantic_analyzer.cpp` — validation | easy |
| 4 | `std_db_instance.cpp::create_table` — auto-index + NOT NULL | medium |
| 5 | `std_db_instance.cpp::insert_row` — NULL guard | easy |
| 6 | `std_storage_serializer.cpp` — serialization | medium |
| 7 | `information_schema_provider.cpp` — expose PK | easy |

---

## What Does Not Need Changing

- `insert_row_into_indexes` — already works for `is_unique`; PK index is picked up automatically
- `Evaluator` / `SemanticAnalyzer::analyze_insert` — NOT NULL is already checked via
  `MetaNotNullConstraint`, which is injected on step 4
- B+ tree — no changes needed
