# DeltaBase — Technical Debt & Improvement Backlog

Items are grouped by category. Within each category, roughly ordered by priority.
Each item includes a **Fix** section with a concrete implementation proposal.

---

## Bugs

### `Evaluator::lt` and `lte` are inverted
**Location:** `src/executor/evaluator.cpp:174–247`

`lt(int, int)` returns `left > right`, and `lte(int, int)` returns `left >= right`. Same for `double`, `string`, `char` overloads.
This means `WHERE age < 5` actually filters `age > 5`, and `lt` / `gr` produce identical results.
`gr` and `gre` are correct. Only `lt` and `lte` are wrong.

**Fix:** One-line change per overload — `left > right` → `left < right` and `left >= right` → `left <= right`.
Four primitive overloads each, eight lines total.

---

### `Evaluator` returns `false` for logical AND / OR
**Location:** `src/executor/evaluator.cpp:58–76`

The `evaluate(DataToken, DataToken, AstOperator)` switch only handles comparison operators
(`EQ`, `NEQ`, `LT`, `LTE`, `GR`, `GRE`, `IS`). Logical operators fall through to `return false`.
Any compound `WHERE a = 1 AND b = 2` will silently evaluate to `false`.

**Fix:** This requires fixing the `Evaluator can only handle leaf AST nodes` architecture issue first
(see Architecture section). Once `evaluate(table, row, BinaryExpr)` recurses into sub-expressions,
`AND`/`OR` can be handled at the top level:

```cpp
case AstOperator::AND:
    return evaluate(table, row, *expr.left) && evaluate(table, row, *expr.right);
case AstOperator::OR:
    return evaluate(table, row, *expr.left) || evaluate(table, row, *expr.right);
```

---

### NULL not propagated in ordered comparisons
**Location:** `src/executor/evaluator.cpp:144–330`

`eq()` correctly handles NULL (returns true only for NULL = NULL). But `lt`, `lte`, `gr`, `gre`
don't check for NULL at all — they proceed to `as<T>()` on an empty `Bytes` buffer.
SQL NULL semantics require ordered comparisons to return UNKNOWN (i.e., filter the row out),
not crash or silently return false.

**Fix:** Add a NULL guard at the top of each `(DataToken, DataToken)` overload, mirroring `eq()`:

```cpp
if (left.type == DataType::_NULL || right.type == DataType::_NULL)
    return false; // UNKNOWN → row excluded
```

---

### Debug string left in semantic analyzer
**Location:** `src/executor/semantic_analyzer.cpp:169`

```cpp
return AnalysisResult("What the fuck is happened");
```
Needs a proper error message.

**Fix:** Replace with a meaningful error such as `"Unsupported statement type"`, or throw
`EngineException` (see Exception Handling section).

---

## Exception Handling

### No unified user-facing exception base class
**Location:** `src/misc/include/exceptions.hpp`, `src/engine/engine.cpp`, `src/cli/cli.cpp`

Currently there are two parallel error-signaling mechanisms:
1. Thrown exceptions — a mix of custom classes (`TableDoesntExist`, `UniqueConstraintViolation`, etc.)
   and raw `std::runtime_error`, all deriving directly from `std::runtime_error` with no common base.
2. `AnalysisResult::err` stores `std::optional<std::runtime_error>` — stores by value, losing
   polymorphism (can't `catch` as `TableDoesntExist` after it's been stored and re-thrown).

Consequence: `Cli::execute_query` has its catch block commented out, so any query error
terminates the process. `execute_meta` catches `std::exception` which also silently swallows
genuine engine bugs.

**Fix:**

1. Introduce `EngineException` as the common base for all expected, user-visible errors:
```cpp
// src/misc/include/exceptions.hpp
class EngineException : public std::runtime_error {
public:
    explicit EngineException(const std::string& msg) : std::runtime_error(msg) {}
};

class TableDoesntExist : public EngineException { ... };
class UniqueConstraintViolation : public EngineException { ... };
// ... all other existing user-facing exceptions inherit EngineException
```

2. Change `AnalysisResult::err` from `std::optional<std::runtime_error>` to
   `std::optional<EngineException>` so the stored type is the correct base.

3. In `Cli::execute_query`, uncomment the catch block and catch only `EngineException`:
```cpp
void Cli::execute_query(...) noexcept(true) {
    try {
        // ...
    } catch (const EngineException& e) {
        io_.write("ERROR: " + std::string(e.what()) + "\n");
    }
    // std::runtime_error and others propagate → crash → visible during dev/testing
}
```

4. In `Cli::execute_meta`, narrow the catch from `std::exception` to `EngineException` for the
   same reason.

This separates "expected SQL errors the user caused" from "engine bugs" — the former are formatted
and shown; the latter crash loudly, which is the right behavior during development.

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

**Fix:** Add a static `DataToken::compare(const DataToken& a, const DataToken& b) -> int`
that dispatches on `a.type` and does a `memcmp`-style comparison on the raw `Bytes` buffer for
fixed-size types (INTEGER, REAL, CHAR, BOOL) and `std::string` compare for TEXT.
Then rewrite `Evaluator::lt/lte/gr/gre` to call `DataToken::compare` and compare the result
to 0, eliminating all the primitive overloads.

---

### `assert()` in `DataToken::as<T>()` silently disappears in Release builds
**Location:** `src/types/data_token.hpp:51, 58, 65, 70`

```cpp
assert(bytes.size() == sizeof(int));
```
In a Release build, this assertion is compiled out. A call with an incorrectly-sized buffer
causes undefined behavior instead of an error. Should throw `std::runtime_error` (or a custom
`CorruptedDataError`) so it fails safely in production.

**Fix:** Replace `assert(bytes.size() == sizeof(T))` with:
```cpp
if (bytes.size() != sizeof(T))
    throw std::runtime_error("DataToken::as<T>: size mismatch");
```
Or introduce a `CorruptedDataError : public std::runtime_error` for I/O-boundary violations.
This is distinct from `EngineException` — it represents internal corruption, not a user error.

---

### No implicit coercion between numeric types
**Location:** `src/executor/evaluator.cpp:85–87`

```cpp
if (left.type != right.type) return false;
```
`5 = 5.0` returns false because `INTEGER != REAL`. Standard SQL allows comparing integers to reals.
The compatibility table in `SemanticAnalyzer` (`src/executor/include/semantic_analyzer.hpp:68–83`)
already knows about compatible types, but the evaluator doesn't use it.

**Fix:** Before the type-equality check in `eq()` (and the other comparison functions), add a
widening step: if one side is `INTEGER` and the other is `REAL`, widen the integer to double
and compare as doubles. This is easiest to add alongside the `DataToken::compare` refactor above.

---

## Performance / Bottlenecks

### O(n) catalog name lookup
**Location:** `src/storage/catalog.cpp:70–79, 109–118`

`CatalogCache::get_table(name, schema_id)` and `get_schema(name)` iterate the entire hash map
linearly. The map is keyed by UUID, so there is no O(1) path for name-based lookup.

Adding a secondary `unordered_map<string, UUID>` for both tables and schemas (updated on `put`)
makes name lookups O(1). This is called on every query.

**Fix:** Add `std::unordered_map<std::string, UUID> table_name_index_` and
`std::unordered_map<std::string, UUID> schema_name_index_` to `CatalogCache`.
Populate them in `put()` and invalidate in `remove()` (if it exists).
`get_table(name, schema_id)` becomes a two-step index lookup instead of a loop.

---

### O(n) column lookup in `MetaTable`
**Location:** `src/types/meta_table.cpp`

`get_column(string)`, `get_column_idx(string)`, and `get_column_idx(ColumnId)` all iterate the
`columns` vector. Called during semantic analysis, planning, and for every row in the evaluator.
A `std::unordered_map<std::string, size_t>` built once at load time would make these O(1).

**Fix:** Add a `std::unordered_map<std::string, size_t> column_index_` field to `MetaTable`,
built in the constructor or a `build_index()` method. `get_column_idx(string)` becomes a single
map lookup. Invalidate/rebuild on `ALTER TABLE ADD COLUMN`.

---

### `output_schema()` recomputed on every call in streaming path
**Location:** `src/executor/node_executor.cpp:44–56, 116–125`

`SeqScanNodeExecutor::output_schema()` calls `db_.get_table()` and rebuilds the vector on every
invocation. It is called by the formatter after all rows are consumed, but nothing stops it from
being called repeatedly. Cache the result after `open()`.

**Fix:** Add `std::optional<OutputSchema> cached_schema_` to `SeqScanNodeExecutor`.
In `open()`, compute and store it. In `output_schema()`, return the cached value
(throw if called before `open()`).

---

### Entire WAL loaded into memory at startup
**Location:** `src/wal/file_wal_manager.cpp`

The WAL manager calls `hydrate_cache()` in the constructor, reading all records into memory.
For a long-running database with a large WAL, this can be gigabytes. Standard practice is to
scan forward from the last checkpoint LSN for recovery, not to load everything.

**Fix:** Remove `hydrate_cache()` from the constructor. During recovery, scan the WAL file
forward from the last checkpoint LSN (stored in the control file or the WAL header).
Only load records needed for the redo/undo pass. Apply and discard as you go; don't accumulate
all records in a vector.

---

### `recursive_mutex` locks the entire `StdDbInstance`
**Location:** `src/storage/include/std_db_instance.hpp:30`

A single `recursive_mutex` serializes all operations on a database instance. Concurrent reads
block each other even though they could proceed in parallel. Moving to a reader-writer lock
(`std::shared_mutex`) would allow concurrent `SELECT` while still serializing writes.

**Fix:** Replace `std::recursive_mutex mtx_` with `std::shared_mutex mtx_`.
Read operations (`get_table`, `seq_scan_begin`, `exists_*`, etc.) acquire `std::shared_lock`.
Write operations (`insert`, `update`, `delete`, `create_table`, `alter_table`, etc.) acquire
`std::unique_lock`. Note: `recursive_mutex` allows re-entrant locking; `shared_mutex` does not,
so any code that locks twice on the same thread needs to be refactored (or a separate
`std::shared_mutex` per table introduced).

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

**Fix:** Change `evaluate(table, row, BinaryExpr)` to visit `expr.left->value` and
`expr.right->value` with `std::visit`. When the variant holds a `BinaryExpr`, recurse.
When it holds a `SqlToken`, proceed as today. This also unblocks AND/OR support (see Bugs section).

```cpp
bool eval_node(const AstNode& node, ...) {
    return std::visit(overloaded{
        [&](const BinaryExpr& e) { return evaluate(table, row, e); },
        [&](const SqlToken& t)   { /* leaf */ }
    }, node.value);
}
```

---

### No query rewrite / optimization pass between planning and execution
**Location:** `src/executor/std_planner.cpp`

Plans are built and immediately executed without any rewriting. Common optimizations that are
absent:
- **Constant folding**: `WHERE 1 = 1` is not simplified away.
- **Predicate pushdown**: filter predicates are not pushed into scan nodes.
- **Dead projection elimination**: selecting only the needed columns is not propagated down.

Even a simple tree-walking rewrite pass before execution would help significantly.

**Fix:** Introduce a `PlanRewriter` interface with a single `rewrite(PlanNode&) -> PlanNode`
method. Start with predicate pushdown: walk the plan tree looking for `FilterNode` above
`SeqScanNode`; merge them into `SeqScanNode::predicate`. Then add constant folding as a
second rewriter pass. Chain rewriters in `std_planner.cpp` before returning the plan.

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

**Fix:** Add a `TableStats` struct to `MetaTable` (or a separate catalog entry) with
`row_count`, `per_column: {min, max, null_count}`. Populate them on `ANALYZE TABLE` (a new
statement). `std_planner.cpp` reads `MetaTable::stats.row_count` instead of the magic threshold,
and estimates selectivity from column min/max when a constant predicate is present.

---

### Raw pointers into `CatalogCache` maps are invalidation-prone
**Location:** `src/storage/catalog.cpp:62–79`, `src/storage/std_db_instance.cpp` (many call sites)

`get_table()` and `get_schema()` return `T*` pointing into `unordered_map` values. An
`unordered_map` rehash (triggered by a `put()`) invalidates all iterators and pointers.
This is an implicit lifetime contract that is not enforced anywhere. The fix is either
returning by value for short-lived uses, or ensuring the map has reserved capacity that
won't trigger rehash.

**Fix (safe, minimal):** Call `reserve(expected_capacity)` on both maps at construction time
to prevent rehash. Document the contract. Longer-term: change `get_table` / `get_schema` to
return `const T&` (reference, not pointer) and `std::nullopt` / throw on miss, so callers
don't store the pointer across mutations.

---

### `InformationSchemaProvider` instantiated fresh in each `VirtualTableNodeExecutor::open()`
**Location:** `src/executor/node_executor.cpp:91`

```cpp
InformationSchemaProvider prov(db_);
```
Created on the stack, used once, discarded. The object is trivial but the pattern doesn't scale
if the provider ever caches anything. The provider should be injected (e.g. passed through
`NodeExecutorFactory`) rather than constructed inline.

**Fix:** Add `InformationSchemaProvider& provider` to `VirtualTableNodeExecutor`'s constructor
(injected by `NodeExecutorFactory`, which already holds a reference to `IDbInstance`).
The factory owns or holds a reference to a single shared `InformationSchemaProvider` instance.

---

### `CatalogSnapshot` version counters are static and unsynchronized
**Location:** `src/types/include/catalog_snapshot.hpp`

`Entry<T>::last_version_` is a `static inline` variable. Multiple threads incrementing it
without a lock will race. Version numbers will be skipped or duplicated, breaking any
optimistic-concurrency logic built on top.

**Fix:** Change `static inline size_t last_version_ = 0` to
`static inline std::atomic<size_t> last_version_ = 0` and use `fetch_add(1)` to increment.
No lock needed; atomic increment is sufficient for a monotonic version counter.

---

### `std::cerr` used directly in `Evaluator` instead of Logger
**Location:** `src/executor/evaluator.cpp:150, 203, 256, 309`

Type mismatch warnings go to `std::cerr` without a timestamp, level, or thread identifier.
In a multi-threaded server these will interleave with other output. Should use `misc::Logger`.

**Fix:** Replace `std::cerr << "..."` with `Logger::warn("...")` (or equivalent). No structural
change needed; just a call-site substitution.

---

## Robustness

### `assert()` as the sole bounds check in release builds
*See also the DataToken section above.*

Beyond `DataToken::as<T>()`, several places in the storage and serializer code use `assert`
for invariants that could be violated by corrupt data on disk. All I/O-boundary checks should
throw, not assert.

**Fix:** Audit all `assert()` calls under `src/storage/` and `src/types/`. Classify each:
- **Programmer invariant** (can never be violated by external input): keep as `assert`.
- **I/O boundary** (could be violated by a corrupt file): replace with `throw CorruptedDataError(...)`.

---

### WAL records are read without checksum validation
**Location:** `src/wal/file_wal_manager.cpp`

Records are deserialized directly with no integrity check. A partially-written or corrupted
WAL record will be applied during recovery, potentially writing garbage into data pages.
A CRC-32 per record (appended at write time, verified at read time) is the standard fix.

**Fix:** When writing a WAL record, append `uint32_t crc = crc32(record_bytes)` after the
payload. When reading during recovery, recompute the CRC and throw `CorruptedDataError` on
mismatch. Stop recovery at the first mismatch — treat the rest of the file as a torn write.
Use `<zlib.h>` (`crc32()`) or a small header-only CRC-32 implementation (no new dependencies
needed if zlib is already linked).

---

### No null-check on schema pointer before dereference in `seq_scan_begin`
**Location:** `src/storage/std_db_instance.cpp:65–66`

`catalog_->get_schema(schema_name)` can return `nullptr`. The next line dereferences it without
a check. If the schema doesn't exist (race, bug, or caller error), this is an immediate crash.

**Fix:** Add a null-check and throw `SchemaDoesntExist(schema_name)` (which becomes an
`EngineException` once that refactor is done). Two lines of code.

---
