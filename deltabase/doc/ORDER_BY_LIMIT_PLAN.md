# Plan: `ORDER BY` + `LIMIT` / `OFFSET`

Step-by-step plan across system layers. Each step is atomic and separately testable.

Status before this work: `LimitPlanNode` and `LimitNodeExecutor` already exist, and
`SelectStmt::limit` is already declared — but the parser never fills the field, so **that
entire code path has never executed**. Step 3 brings it to life for the first time; expect
the first bugs there.

---

## Design decisions

Three decisions must be settled before writing code.

### D1 — Where `Sort` sits in the plan tree

```
Scan → Filter → Sort → Project → Limit
```

`Sort` goes **below** `Project`, not above it. That way `ORDER BY` sees the full table row
and can order by a column absent from the select list (`SELECT name FROM t ORDER BY age`),
and key column indices resolve through `MetaTable::get_column_idx()` — exactly as
`FilterNodeExecutor` and `ProjectionNodeExecutor::open()` (`node_executor.cpp:209`) already do.

Cost: wider rows get sorted. The alternative (`Sort` above `Project`) forbids ordering by a
non-projected column and buys nothing, because the engine supports neither aliases nor
expressions in the select list.

### D2 — `Sort` is a pipeline breaker

The Volcano model (`open`/`next`/`close`) is pull-based: `next()` yields **one** row, and the
caller asks for the next one when ready. `SeqScan`, `Filter`, `Project` and `Limit` therefore
hold O(1) memory and make the first row available immediately, without reading the table to
the end.

`Sort` cannot do that. To return the **first** output row it must know the minimum, and the
minimum may live in the last row of the table. So no answer can be produced before the child
is fully drained: `open()` loops `child_->next()` until `false`, collects everything into a
`std::vector<DataRow>`, sorts, and only then does `next()` start serving from the vector by
index.

An operator that must consume all its input before producing any output is a *pipeline
breaker*. Consequences specific to this engine:

- memory is O(n) instead of O(1) — the whole result lives in RAM;
- latency to the first row equals a full scan;
- `StreamedResult` over such a plan is pointless (hence `needs_stream = false` when a `Sort`
  is present) — and since locking is strict 2PL at whole-database granularity, the lock is
  held for the entire drain anyway, so streaming would only prolong it;
- `LIMIT` above `Sort` stops saving I/O: previously `LIMIT 10` could abort the scan at row 10,
  but now the sort underneath reads the whole table regardless. The saving remains only on
  the output side.

This is normal and unavoidable — in every database `Sort` is a blocking operator. What is
*not* inevitable is unbounded memory. Real systems break the pipeline the same way but cap
the cost:

- **External merge sort.** When the input exceeds the memory budget, sort in chunks, spill
  sorted runs to disk, then merge. In PostgreSQL the budget is `work_mem`, and
  `EXPLAIN ANALYZE` reports `Sort Method: quicksort Memory: 25kB` versus
  `Sort Method: external merge Disk: 2000kB`.
- **Top-K instead of a full sort.** With `LIMIT n` above the sort, sorting everything is
  wasteful — keep a heap of `n + offset` elements: O(n) memory and O(N log n) time instead of
  O(N log N). PostgreSQL calls this `top-N heapsort`. This is the most valuable optimization
  here, because `ORDER BY ... LIMIT 10` is exactly the query worth demonstrating.
- **Eliminating the sort entirely** via an ordered index scan, when an index already supplies
  the required order (see D3 and step 6 for why this is restricted to `NOT NULL` columns).

**Decision for v1:** in-memory sort, no spilling, no top-K. The limitation ("the result set
must fit in RAM") is stated explicitly, and external merge sort plus top-K are recorded as
known follow-ups. Note that the engine has no notion of a memory budget at all, so there is
currently nothing to check "fits / does not fit" against — that is deliberate debt.

### D3 — NULL ordering: a correctness trap, not a cosmetic choice

`std::sort` requires its comparator to be a **strict weak ordering**. One requirement concerns
transitivity not of the comparison itself but of the *induced equivalence*: if
`!comp(a,b) && !comp(b,a)` then `a` and `b` count as equal, and that equality relation must be
transitive.

Now consider `operator<` at `data_token.cpp:215`:

```cpp
if (lhs.type == DataType::_NULL || rhs.type == DataType::_NULL) return false;
```

For `WHERE` this is correct — SQL mandates UNKNOWN and the row is filtered out. But used as a
sort comparator, take `a = NULL`, `b = 5`, `c = 7`:

- `a < b` → false, `b < a` → false ⟹ NULL "equals" 5
- `a < c` → false, `c < a` → false ⟹ NULL "equals" 7
- `b < c` → **true** ⟹ 5 does **not** equal 7

Equality is not transitive. The contract is violated.

Why this is undefined behaviour rather than merely a wrong row order: libstdc++'s `std::sort`
is an introsort whose partition loop reads `while (comp(*first, pivot)) ++first;` — **with no
bounds check**. Its correctness relies on the guarantee that somewhere past the pivot an
element exists for which the comparator returns false and the loop stops. Break the contract
and that sentinel is gone, the pointer walks off the buffer, and the result is memory
corruption or a segfault. It also manifests non-deterministically, depending on where the NULL
landed relative to the pivot. This is expensive to debug, so it must be right the first time.

**Decision:** the sort comparator must not use `operator<` or raw `DataToken::compare`. It
ranks NULLs first — assign each token rank `0` when a value is present and `1` when the type
is `_NULL` (which yields NULLS LAST for ASC). Compare ranks; if both are `1` the tokens are
equivalent and the next key is consulted; if both are `0`, call `DataToken::compare`. NULL is
then equivalent to other NULLs and to nothing else, and transitivity holds.

Sketch, for a single key:

```
rank(t) = (t.type == _NULL) ? 1 : 0
if (rank(l) != rank(r)) return rank(l) - rank(r);   // NULLs sink to the bottom
if (rank(l) == 1)        return 0;                  // both NULL — equivalent
return DataToken::compare(l, r);
```

The `DESC` inversion applies to the whole per-key result, so NULLs float to the top
automatically — NULLS FIRST for DESC, matching PostgreSQL.

**No wrapper around `std::sort` is needed.** `std::sort` / `std::stable_sort` take the
comparator as their third argument; what gets fixed is the *comparator*, at the level of a
single two-token comparison. No wrapper, no extra pass over the data.

The remaining question is **where that comparator lives** — and it is not the sort executor.
Ordering is a property of the type system, not of one operator: the same total order will be
needed by `GROUP BY`, merge join and ordered index scan. So it belongs next to `DataToken`, as
a separate function such as `compare_ordered(a, b)`, while the sort node merely loops over
keys and flips the sign for `DESC`.

The conceptual rule that follows: the engine must host **two distinct notions of comparison**,
and they must never be conflated.

1. **SQL three-valued logic** — for `WHERE`. `NULL = NULL` → UNKNOWN, row discarded. These are
   the existing `operator==` / `operator<`, and they must not be touched, or predicates break.
2. **Total order** — for sorting and indexes. NULL must have a definite position: equal to
   other NULLs, unequal to everything else.

PostgreSQL draws exactly the same line: the `=` operator yields UNKNOWN on NULL, while sorting
and btree use separate semantics (`IS DISTINCT FROM`, `NULLS FIRST/LAST`). This is a good
answer to "why do you have two comparison functions?" at the defence.

Note that the second order already exists in this codebase, and it is built incorrectly —
`IndexBPlusTree::compare_token` (`index_bplus_tree.cpp:239`):

```cpp
if (a == b) return 0;
return a < b ? -1 : 1;
```

For NULL against a number both `==` and `<` return false, so `compare_token(NULL, 5)` is `1`
**and** `compare_token(5, NULL)` is `1`: each "greater than" the other. That asymmetry would
corrupt tree descent (the loops at `index_bplus_tree.cpp:193` and `:211`). It does not fire
today only because NULLs never reach the index at all — `dml_service.cpp:87` skips them on key
insertion.

Two consequences:

- Extracting `compare_ordered` into `types` and reusing it inside `compare_token` makes both
  sites consistent and removes a latent bug.
- **An ordered index scan cannot replace a `Sort` on a nullable column** — the index simply
  does not contain the NULL rows, so the result would silently lose them. That optimization is
  valid only when the column is `NOT NULL` (see step 6).

---

## Phase 1 — SQL layer

### 1.1 `types/include/sql_token.hpp`, `sql/include/dictionary.hpp`

Add to the `SqlKeyword` enum (`sql_token.hpp:25`) and to `keywords_map()`
(`dictionary.hpp:19`):

```
order  → SqlKeyword::ORDER
by     → SqlKeyword::BY
asc    → SqlKeyword::ASC
desc   → SqlKeyword::DESC
limit  → SqlKeyword::LIMIT
offset → SqlKeyword::OFFSET
```

> Side effect: `order`, `by`, `limit`, `offset`, `asc`, `desc` become reserved words, so a
> column with one of those names will no longer parse. Acceptable, but worth knowing.

### 1.2 `types/include/ast_tree.hpp`

Add:

```cpp
struct OrderByItem {
    SqlToken column;
    bool desc = false;
};
```

Extend `SelectStmt` (`ast_tree.hpp:124`) with `std::vector<OrderByItem> order_by;` and
`std::optional<uint64_t> offset;`. The `limit` field already exists (`ast_tree.hpp:129`).

### 1.3 `sql/parser.cpp` — `parse_select()`

Parse the new clauses after the `WHERE` block (`parser.cpp:181`), in the order
`ORDER BY` → `LIMIT` → `OFFSET`. Grammar to support:

```sql
SELECT ... [WHERE ...] [ORDER BY col [ASC|DESC] {, col [ASC|DESC]}] [LIMIT n] [OFFSET m]
```

`ASC` is the default; direction is per column. `OFFSET` must be accepted without `LIMIT`.

Pitfalls:

- **Cursor discipline.** `parse()` ends by calling `is_trailing_sequence_valid`
  (`parser.cpp:14`), which accepts the cursor at `current_` **or** `current_ + 1`. If the new
  clauses leave the cursor anywhere else, a valid query fails with "Unexpected tokens after
  statement". Establish first where the cursor lands after `parse_binary_expr(0)` in the
  `WHERE` branch, then normalize the position before parsing `ORDER BY`.
- The `LIMIT` / `OFFSET` value is a `SqlTokenType::LITERAL` token with detail
  `SqlLiteral::INTEGER`; parse with `std::stoull` and reject negative or non-integer values in
  the parser itself.

---

## Phase 2 — Semantic analysis

### 2.1 `executor/semantic_analyzer.cpp` — `analyze_select()`

In `analyze_select` (`semantic_analyzer.cpp:67`) add a loop over `order_by` validating each
column with `table->has_column()` — a copy of the loop for `stmt.columns` at line 90.
`has_column` already skips dropped columns, so no separate `is_dropped` check is needed.

This step is not optional. `ProjectionNodeExecutor::open()` validates its resolved index with
`assert(idx >= 0)` (`node_executor.cpp:210`), which is compiled out in Release builds. The
sort node resolves indices the same way, so an unvalidated `ORDER BY` column would produce not
an error but undefined behaviour.

Works for virtual tables too (`information_schema`), where `table` points at the virtual
`MetaTable`.

---

## Phase 3 — Planner

### 3.1 `types/include/query_plan.hpp`

- Add `SORT` to `IPlanNode::Type` (line 22).
- Add `SortPlanNode : UnaryPlanNode` holding `std::vector<SortKey>` (where `SortKey` is
  `{std::string column; bool desc;}`) and a `MetaTable` copy — mirroring `FilterPlanNode`
  (line 157), so the executor can resolve indices.
- Extend `LimitPlanNode` (line 207) with `uint64_t offset`, and make `limit` a
  `std::optional<uint64_t>` — otherwise `OFFSET` without `LIMIT` is inexpressible.

### 3.2 `executor/std_planner.cpp` — `plan(SelectStmt&)`

In `plan(SelectStmt&)` (`std_planner.cpp:231`) insert the `Sort` node between step "2. WHERE"
and step "3. PROJECT". Step "4. LIMIT" builds its node when `limit || offset`.

Also:

- `estimate_seq_scan_selectivity` (line 28) needs a `SORT` case (pass-through: child
  selectivity, cardinality unchanged). Otherwise `default: return 1.0` silently discards the
  whole estimate from below.
- Rework the `LIMIT` case (line 54) for `offset` (`limit + offset` rows must be scanned) and
  for `limit` now being optional.
- Set `needs_stream = false` whenever a `Sort` is present (see D2). The condition at line 314
  is in fact always false for `SELECT` today — after `Project` is added the plan root is never
  `SEQ_SCAN`. That is a separate discussion; do not fix it here.
- **Future optimization, not in v1:** if the single `ORDER BY` column has a B+ tree index and
  the direction is ASC, the index scan already returns rows in order and the `Sort` can be
  dropped entirely. Valid only for `NOT NULL` columns — see D3. Good follow-up and a strong
  point at the defence.

---

## Phase 4 — Executor

### 4.1 `executor/include/node_executor.hpp`, `executor/node_executor.cpp`

`SortNodeExecutor`:

- fields: `MetaTable`, keys, child, `std::vector<DataRow> rows_`, `size_t idx_`;
- `open()` — `child_->open()`, resolve key column indices once via `table_.get_column_idx`
  (as in `ProjectionNodeExecutor::open()`, `node_executor.cpp:209`), drain the child into
  `rows_`, then `std::stable_sort` with the multi-key comparator. Stability makes the row
  order deterministic on equal keys, which matters for tests;
- `next()` — serve from `rows_` by `idx_`;
- `output_schema()` — delegate to the child.

Comparator: iterate the keys in order, apply the NULL rank rule from D3 before calling
`DataToken::compare`, invert the per-key result for `desc`, and return on the first non-zero.

`LimitNodeExecutor` (`node_executor.cpp:253`): skip `offset_` rows from the child on the first
`next()` call, then keep the existing counter logic. Update the constructor signature.

### 4.2 `NodeExecutorFactory::from_plan`

Add `case IPlanNode::Type::SORT` (`node_executor.cpp:733`).

### 4.3 Sweep for exhaustive switches

Grep for `IPlanNode::Type::LIMIT` to find any other switch over the plan node type (a plan
printer, for instance). `result_formatter` needs no changes.

---

## Phase 5 — Tests

Only now can exact row order be asserted, which makes this the first real opportunity to write
proper integration tests for `SELECT`. Minimum coverage:

- ASC / DESC; multiple columns with mixed directions
- `ORDER BY` on a column absent from the select list
- NULL placement in both directions
- `LIMIT 0`; `LIMIT` greater than the row count; `OFFSET` past the end; `OFFSET` without
  `LIMIT`; both together
- `ORDER BY` + `LIMIT` combined — the classic bug is applying the limit before the sort
- `ORDER BY` on a column after `DROP COLUMN` (index shift — `is_dropped`)
- `ORDER BY` on `information_schema.tables`

---

## Work order

1. Steps 1.1 → 1.2 → 1.3, then build and verify that existing queries did not regress — the
   cursor problem surfaces exactly here.
2. Step 2.1 (semantics before the executor, so an invalid column never reaches the
   compiled-out `assert`).
3. Steps 3.1 → 3.2 → 4.1 → 4.2 → 4.3.
4. Phase 5.

Suggested commits:

- (a) keywords + AST + parser
- (b) plan node + planner + executor + semantic analysis
- (c) tests

---

## Known limitations after this work

- The sort is in-memory; the result set must fit in RAM. No external merge sort.
- No top-K optimization for `ORDER BY ... LIMIT n`; the full result is always sorted.
- No ordered index scan, so `ORDER BY` on an indexed column still sorts.
- `order`, `by`, `limit`, `offset`, `asc`, `desc` are now reserved and unusable as identifiers.
