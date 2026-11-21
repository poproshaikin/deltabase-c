# Plan: aggregates, `GROUP BY`, `HAVING`

Step-by-step plan across system layers. Each step is atomic and separately testable.

The counting itself is the easy part. What costs is that the select list stops being a list
of column names, and that everything above an aggregate works on a new row shape rather than
on table rows.

Three places read `SelectStmt::columns` for a SELECT, so the ripple is small:
`sql/parser.cpp:199`, `executor/semantic_analyzer.cpp:92`, `executor/std_planner.cpp:329`.
The other hits for `.columns` belong to `INSERT` and `CREATE TABLE`.

---

## Design decisions

### D1 — Aggregate names stay identifiers, not keywords

`count`, `sum`, `avg`, `min`, `max` are all plausible column names, and `ORDER BY` has
already taken six words out of circulation. Making five more reserved is a poor trade, and
PostgreSQL does not reserve them either.

So the lexer keeps producing `IDENTIFIER` for them, and the parser recognises an aggregate
the way SQL does: **an identifier immediately followed by `(`**. Only the clause introducers
`GROUP` and `HAVING` become keywords. `BY` already exists from `ORDER BY`.

### D2 — `GROUP BY` groups by sorting, not by hashing

The obvious implementation is a hash table keyed by the group key values. But there is no
hash function for `DataToken`, and writing one means handling every type plus NULL.

Sorting is already available and tested, and `DataToken::compare_ordered` already gives a
total order over key values. So grouping sorts by the group keys and then walks the sorted
input once, emitting a row whenever the key changes:

```
Scan → Filter → Sort(group keys) → GroupAggregate
```

This is PostgreSQL's `GroupAggregate` as opposed to `HashAggregate`, so the choice is a
recognisable one rather than an improvisation: hashing is faster, sorting needs no new type
machinery and yields groups in a deterministic order. Hash-based grouping stays available
later as an optimization, exactly as hash join does for nested-loop join.

Scalar aggregates — no `GROUP BY` — need no sort at all: one accumulator, one output row.

### D3 — An aggregate node produces the final row shape, and nothing above resolves names

This is the decision that bounds the work.

`ProjectPlanNode` and `ProjectionNodeExecutor` resolve column names against `MetaTable`.
`OrderByNodeExecutor` does the same. Neither can resolve `count`, because no such column
exists in the table. Above an aggregate, rows are no longer table rows: they are group keys
plus aggregate values.

Therefore:

- when a query has aggregates, the aggregate node emits exactly the select list, and **no
  `Project` is placed above it**;
- `ORDER BY` over an aggregate query is **rejected in v1**. Supporting it means teaching the
  sort to address output positions instead of table columns, which is a separate change;
- `LIMIT` / `OFFSET` work unchanged — the limit node resolves no names.

v1 pipelines:

```
no GROUP BY:   Scan → Filter → ScalarAggregate → Limit
with GROUP BY: Scan → Filter → Sort(keys) → GroupAggregate → Filter(HAVING) → Limit
```

### D4 — NULL and empty-input rules are part of the specification

Each is one line of code and each fails silently when wrong, so they are written down here
rather than discovered later:

| Case | Result |
|---|---|
| `COUNT(*)` | counts rows, NULLs included |
| `COUNT(x)` | counts rows where `x` is not NULL |
| `SUM`, `AVG` | skip NULL inputs |
| `SUM` over no input | NULL, not 0 |
| `COUNT` over no input | 0 |
| `AVG` | always `REAL`, even over `INTEGER` |
| `MIN`, `MAX` | ignore NULL; compare with `compare_ordered` |
| every aggregate over an empty group | the group does not exist, so no row |
| scalar aggregate over an empty table | one row: `COUNT` is 0, the rest are NULL |

### D5 — Mixing bare columns with aggregates is an error without `GROUP BY`

`select name, count(*) from t` has no meaning: one value for `name`, one row for the whole
table. It must be rejected unless every bare column also appears in `GROUP BY`. Easy to
forget, and without the check the engine returns an arbitrary row.

---

## Phase 1 — SQL layer

### 1.1 `types/include/sql_token.hpp`, `sql/include/dictionary.hpp`

Add `GROUP` and `HAVING` to `SqlKeyword` and to `keywords_map()`. Nothing else: per D1 the
aggregate names stay identifiers.

### 1.2 `types/include/ast_tree.hpp`

```cpp
enum class AggregateKind { COUNT_STAR, COUNT, SUM, AVG, MIN, MAX };

struct AggregateCall
{
    AggregateKind kind;
    SqlToken column;            // unused for COUNT_STAR
};

struct SelectItem
{
    std::variant<SqlToken, AggregateCall> value;
};
```

`SelectStmt::columns` becomes `std::vector<SelectItem>`, and the statement gains
`std::vector<SqlToken> group_by` and `std::optional<AggregateCall> having_subject` plus the
operator and literal for the `HAVING` comparison (see 1.4).

### 1.3 `sql/parser.cpp` — the select list

The column loop (`parser.cpp:186`) currently pushes an identifier per iteration. It now
reads one `SelectItem`: an identifier, and if the next token is `(`, an aggregate call —
`name ( * )` for `COUNT`, `name ( identifier )` otherwise.

Reject unknown function names here, so `foo(x)` fails in the parser rather than reaching
the analyzer as a mystery.

The strictness added for `ORDER BY` must survive: anything that is neither an identifier
nor `*` is still a syntax error.

### 1.4 `sql/parser.cpp` — `GROUP BY` and `HAVING`

`GROUP BY` is a comma-separated identifier list, parsed exactly like `ORDER BY` minus the
direction, between `WHERE` and `ORDER BY`.

`HAVING` is a predicate over aggregate results, and the general expression parser cannot
produce one: it builds `BinaryExpr` over identifiers and literals, with no notion of a call.
Teaching it calls is a larger change than this feature needs, so v1 accepts only the single
form

```
HAVING <aggregate call> <comparison operator> <literal>
```

parsed directly, not through `parse_binary_expr`. `HAVING count(*) > 5` works;
`HAVING count(*) > avg(x)` does not. Written down as a limitation rather than hidden.

Cursor discipline is the same trap as before: each clause leaves the cursor past itself, or
on its last token at end of input.

---

## Phase 2 — Semantic analysis

### 2.1 `executor/semantic_analyzer.cpp` — `analyze_select`

The loop over `stmt.columns` (line 92) now visits a variant. For a bare column, the existing
`has_column` check. For an aggregate:

- the argument column exists (`COUNT_STAR` has none);
- `SUM` and `AVG` require a numeric column — `INTEGER` or `REAL`. `MIN`, `MAX` and `COUNT`
  accept any type.

Then the rules from D5 and D3:

- every bare column in the select list appears in `GROUP BY`, otherwise reject;
- each `GROUP BY` column exists;
- `ORDER BY` together with aggregates is rejected with "not supported yet";
- `HAVING` without any aggregate in the query is rejected.

---

## Phase 3 — Planner

### 3.1 `types/include/query_plan.hpp`

Add `AGGREGATE` to `IPlanNode::Type` and

```cpp
struct AggregatePlanNode final : UnaryPlanNode
{
    MetaTable table;                      // to resolve argument and key columns
    std::vector<AggregateCall> aggregates;
    std::vector<std::string> group_by;    // empty for a scalar aggregate
    ...
};
```

### 3.2 `executor/std_planner.cpp` — `plan(SelectStmt&)`

Between the `WHERE` step and the `PROJECT` step:

- if `group_by` is non-empty, build an `OrderByPlanNode` over the group keys (ascending,
  `fetch` empty — a bound would cut groups off);
- build the `AggregatePlanNode`;
- **skip the `PROJECT` step entirely** when the query has aggregates (D3);
- `HAVING` becomes a filter above the aggregate;
- `LIMIT` / `OFFSET` as they are now.

`estimate_seq_scan_selectivity` needs an `AGGREGATE` case. Cardinality drops to the number
of groups, which is unknown without statistics; for a scalar aggregate it is exactly one row.
Until statistics exist, return something defensible and say so in a comment — one row for a
scalar aggregate, and the child's estimate for a grouped one.

---

## Phase 4 — Executor

### 4.1 `executor/node_executor.cpp` — `AggregateNodeExecutor`

One accumulator per aggregate, reset per group:

```cpp
struct Accumulator
{
    uint64_t count = 0;        // rows counted
    DataToken running;         // sum, min or max so far
    bool seen = false;         // distinguishes "no input" from a zero result
};
```

`open()` — resolve argument and key column indices once (as `OrderByNodeExecutor` does),
then:

- **scalar**: drain the child, feeding every row into the accumulators, and build the single
  output row;
- **grouped**: the input arrives sorted by the keys, so read rows one at a time, and whenever
  the key differs from the previous one, emit the finished group and reset. Only the current
  group's accumulators are held, so memory is O(1) in the number of groups — the advantage of
  sorting over hashing, worth stating at a defence.

`next()` serves the finished rows. `output_schema()` is built synthetically: names from the
select list (`count`, `sum`, or the column name for a bare key) and types per D4 — `COUNT`
is `INTEGER`, `AVG` is `REAL`, `MIN` and `MAX` take the column's type, `SUM` keeps
`INTEGER` or widens to `REAL`.

`MIN` and `MAX` use `DataToken::compare_ordered`. `SUM` and `AVG` need numeric addition on
`DataToken`, which does not exist yet — a small helper next to `compare_ordered`, with the
same widening rule: `INTEGER + INTEGER` stays `INTEGER`, anything with a `REAL` becomes
`REAL`.

Account the output rows against `ctx_.budget` with a `MemoryReservation`, as the sort does.
A grouped aggregate holds only one group at a time, but the finished rows accumulate.

### 4.2 `NodeExecutorFactory::from_plan`

Add `case IPlanNode::Type::AGGREGATE`.

---

## Phase 5 — Tests

- each aggregate over a known table, checked against hand-computed values
- every row of D4: `COUNT(*)` versus `COUNT(x)` with NULLs, `SUM` over no rows, `AVG` of
  integers, `MIN`/`MAX` with NULLs present, a scalar aggregate over an empty table
- `GROUP BY` with one and several keys; groups of size one; a key that is NULL
- `GROUP BY` where every row is its own group, and where all rows are one group
- `HAVING` filtering groups out, and filtering all of them out
- `LIMIT` over a grouped result
- rejected: `select name, count(*)` without `GROUP BY`; `SUM` over `TEXT`; `ORDER BY` with
  aggregates; an unknown function name
- the same queries with `page_cache_size = 2`, since grouping reads the table through a sort

---

## Work order

1. 1.1 → 1.2 → 1.3, then build and check that existing queries still parse. The select-list
   change touches the one loop every `SELECT` goes through, so regressions surface here.
2. Phase 2, before the executor: a wrong column must not reach the compiled-out `assert`.
3. 3.1 → 4.1 → 4.2 for scalar aggregates only, and get `select count(*) from t` working end
   to end.
4. `GROUP BY`: the sort in 3.2 and the group loop in 4.1.
5. `HAVING`.
6. Phase 5 alongside each step, not at the end.

Suggested commits: (a) select list as items plus scalar aggregates, (b) `GROUP BY`,
(c) `HAVING`, (d) tests if they are not carried along.

---

## Known limitations after this work

- `ORDER BY` cannot be combined with aggregates (D3).
- `HAVING` accepts only `<aggregate> <op> <literal>` (1.4).
- No aggregate expressions: `sum(x) / count(*)`, `count(*) + 1` are not expressible.
- No `DISTINCT` inside an aggregate: `count(distinct x)`.
- No aliases, so output columns are named after the function.
- Grouping always sorts; no `HashAggregate`.
- The planner cannot estimate the number of groups, so a grouped aggregate's cardinality
  estimate is a placeholder until statistics exist.
