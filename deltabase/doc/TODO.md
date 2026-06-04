# DeltaBase — Technical Debt & Improvement Backlog

Items are grouped by category. Within each category, roughly ordered by priority.

---

## Bugs

### `Evaluator::lt` and `lte` are inverted
**Location:** `src/executor/evaluator.cpp:174–247`

`lt(int, int)` returns `left > right`, and `lte(int, int)` returns `left >= right`. Same for `double`, `string`, `char` overloads.
This means `WHERE age < 5` actually filters `age > 5`, and `lt` / `gr` produce identical results.
`gr` and `gre` are correct. Only `lt` and `lte` are wrong.

---

### `Evaluator` returns `false` for logical AND / OR
**Location:** `src/executor/evaluator.cpp:58–76`

The `evaluate(DataToken, DataToken, AstOperator)` switch only handles comparison operators
(`EQ`, `NEQ`, `LT`, `LTE`, `GR`, `GRE`, `IS`). Logical operators fall through to `return false`.
Any compound `WHERE a = 1 AND b = 2` will silently evaluate to `false`.

---

### NULL not propagated in ordered comparisons
**Location:** `src/executor/evaluator.cpp:144–330`

`eq()` correctly handles NULL (returns true only for NULL = NULL). But `lt`, `lte`, `gr`, `gre`
don't check for NULL at all — they proceed to `as<T>()` on an empty `Bytes` buffer.
SQL NULL semantics require ordered comparisons to return UNKNOWN (i.e., filter the row out),
not crash or silently return false.

---

### Debug string left in semantic analyzer
**Location:** `src/executor/semantic_analyzer.cpp:169`

```cpp
return AnalysisResult("What the fuck is happened");
```
Needs a proper error message.

---

## Type System

### `DataToken` comparisons go through C++ native types
**Location:** `src/types/data_token.hpp`, `src/executor/evaluator.cpp`

Every comparison deserializes both operands into a C++ primitive (`as<int>()`, `as<double>()`, etc.)
and then uses the built-in C++ operator. This tightly couples the type system to C++ types and
forces a round-trip through `memcpy` for every comparison.

A cleaner design is type-aware byte-level comparison functions that operate directly on `Bytes`:

```cpp
// instead of: left.as<int>() < right.as<int>()
// something like: DataToken::compare(left, right) < 0
```

This would decouple the storage representation from the comparison logic, make it easier to add
new types (e.g. DECIMAL, DATE), and eliminate the repeated `memcpy` + cast overhead.

---

### `assert()` in `DataToken::as<T>()` silently disappears in Release builds
**Location:** `src/types/data_token.hpp:51, 58, 65, 70`

```cpp
assert(bytes.size() == sizeof(int));
```
In a Release build, this assertion is compiled out. A call with an incorrectly-sized buffer
causes undefined behavior instead of an error. Should throw `std::runtime_error` (or a custom
`CorruptedDataError`) so it fails safely in production.

---

### No implicit coercion between numeric types
**Location:** `src/executor/evaluator.cpp:85–87`

```cpp
if (left.type != right.type) return false;
```
`5 = 5.0` returns false because `INTEGER != REAL`. Standard SQL allows comparing integers to reals.
The compatibility table in `SemanticAnalyzer` (`src/executor/include/semantic_analyzer.hpp:68–83`)
already knows about compatible types, but the evaluator doesn't use it.

---

## Performance / Bottlenecks

### O(n) catalog name lookup
**Location:** `src/storage/catalog.cpp:70–79, 109–118`

`CatalogCache::get_table(name, schema_id)` and `get_schema(name)` iterate the entire hash map
linearly. The map is keyed by UUID, so there is no O(1) path for name-based lookup.

Adding a secondary `unordered_map<string, UUID>` for both tables and schemas (updated on `put`)
makes name lookups O(1). This is called on every query.

---

### O(n) column lookup in `MetaTable`
**Location:** `src/types/meta_table.cpp`

`get_column(string)`, `get_column_idx(string)`, and `get_column_idx(ColumnId)` all iterate the
`columns` vector. Called during semantic analysis, planning, and for every row in the evaluator.
A `std::unordered_map<std::string, size_t>` built once at load time would make these O(1).

---

### `output_schema()` recomputed on every call in streaming path
**Location:** `src/executor/node_executor.cpp:44–56, 116–125`

`SeqScanNodeExecutor::output_schema()` calls `db_.get_table()` and rebuilds the vector on every
invocation. It is called by the formatter after all rows are consumed, but nothing stops it from
being called repeatedly. Cache the result after `open()`.

---

### Entire WAL loaded into memory at startup
**Location:** `src/wal/file_wal_manager.cpp`

The WAL manager calls `hydrate_cache()` in the constructor, reading all records into memory.
For a long-running database with a large WAL, this can be gigabytes. Standard practice is to
scan forward from the last checkpoint LSN for recovery, not to load everything.

---

### `recursive_mutex` locks the entire `StdDbInstance`
**Location:** `src/storage/include/std_db_instance.hpp:30`

A single `recursive_mutex` serializes all operations on a database instance. Concurrent reads
block each other even though they could proceed in parallel. Moving to a reader-writer lock
(`std::shared_mutex`) would allow concurrent `SELECT` while still serializing writes.

---

## Architecture & Coupling

### `Evaluator` can only handle leaf AST nodes (not nested `BinaryExpr`)
**Location:** `src/executor/evaluator.cpp:20–21`

```cpp
auto left = std::get<SqlToken>(expr.left->value);
auto right = std::get<SqlToken>(expr.right->value);
```
Both sides are unconditionally cast to `SqlToken`. If either side is itself a `BinaryExpr`
(e.g. `WHERE (a > 1) AND (b < 2)`), this throws. The evaluator needs to recursively handle
the AST instead of assuming flat structure.

---

### No query rewrite / optimization pass between planning and execution
**Location:** `src/executor/std_planner.cpp`

Plans are built and immediately executed without any rewriting. Common optimizations that are
absent:
- **Constant folding**: `WHERE 1 = 1` is not simplified away.
- **Predicate pushdown**: filter predicates are not pushed into scan nodes.
- **Dead projection elimination**: selecting only the needed columns is not propagated down.

Even a simple tree-walking rewrite pass before execution would help significantly.

---

### Selectivity estimates are magic constants
**Location:** `src/executor/std_planner.cpp:19–21`

```cpp
constexpr double k_default_filter_selectivity = 0.3;
constexpr double k_seq_scan_stream_threshold_rows = 2048.0;
```
The planner has no access to actual table statistics. For tables with 100% or 0% selectivity,
streaming decisions will be wrong. Even basic stats (min/max per column, histogram buckets)
stored in `MetaTable` would allow data-driven estimates.

---

### Raw pointers into `CatalogCache` maps are invalidation-prone
**Location:** `src/storage/catalog.cpp:62–79`, `src/storage/std_db_instance.cpp` (many call sites)

`get_table()` and `get_schema()` return `T*` pointing into `unordered_map` values. An
`unordered_map` rehash (triggered by a `put()`) invalidates all iterators and pointers.
This is an implicit lifetime contract that is not enforced anywhere. The fix is either
returning by value for short-lived uses, or ensuring the map has reserved capacity that
won't trigger rehash.

---

### `InformationSchemaProvider` instantiated fresh in each `VirtualTableNodeExecutor::open()`
**Location:** `src/executor/node_executor.cpp:91`

```cpp
InformationSchemaProvider prov(db_);
```
Created on the stack, used once, discarded. The object is trivial but the pattern doesn't scale
if the provider ever caches anything. The provider should be injected (e.g. passed through
`NodeExecutorFactory`) rather than constructed inline.

---

### `CatalogSnapshot` version counters are static and unsynchronized
**Location:** `src/types/include/catalog_snapshot.hpp`

`Entry<T>::last_version_` is a `static inline` variable. Multiple threads incrementing it
without a lock will race. Version numbers will be skipped or duplicated, breaking any
optimistic-concurrency logic built on top.

---

### `std::cerr` used directly in `Evaluator` instead of Logger
**Location:** `src/executor/evaluator.cpp:150, 203, 256, 309`

Type mismatch warnings go to `std::cerr` without a timestamp, level, or thread identifier.
In a multi-threaded server these will interleave with other output. Should use `misc::Logger`.

---

## Robustness

### `assert()` as the sole bounds check in release builds
*See also the DataToken section above.*

Beyond `DataToken::as<T>()`, several places in the storage and serializer code use `assert`
for invariants that could be violated by corrupt data on disk. All I/O-boundary checks should
throw, not assert.

---

### WAL records are read without checksum validation
**Location:** `src/wal/file_wal_manager.cpp`

Records are deserialized directly with no integrity check. A partially-written or corrupted
WAL record will be applied during recovery, potentially writing garbage into data pages.
A CRC-32 per record (appended at write time, verified at read time) is the standard fix.

---

### No null-check on schema pointer before dereference in `seq_scan_begin`
**Location:** `src/storage/std_db_instance.cpp:65–66`

`catalog_->get_schema(schema_name)` can return `nullptr`. The next line dereferences it without
a check. If the schema doesn't exist (race, bug, or caller error), this is an immediate crash.
