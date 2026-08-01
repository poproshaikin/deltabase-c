# Plan: Foreign Key (ON DELETE CASCADE / SET NULL)

Пошаговый план по слоям системы. Каждый шаг атомарный и тестируемый отдельно.

---

## Phase 1 — SQL Layer (Lexer, Dictionary, AST, Parser)

### 1.1 `sql/include/dictionary.hpp`

Добавить ключевые слова в `SqlKeyword` enum и `keywords_map()`:

```
foreign    → SqlKeyword::FOREIGN
references → SqlKeyword::REFERENCES
cascade    → SqlKeyword::CASCADE
restrict   → SqlKeyword::RESTRICT
action     → SqlKeyword::ACTION
```

> `on`, `set`, `delete`, `no` уже есть в словаре — не трогать.  
> Добавить `foreign` и `references` в `constraints_map()` / `is_constraint_kw()`.

### 1.2 `types/include/ast_tree.hpp`

Добавить:

```cpp
enum class OnDeleteAction { NO_ACTION, RESTRICT, CASCADE, SET_NULL };

struct ForeignKeyConstraint {
    SqlToken referenced_table;   // обязательное
    SqlToken referenced_column;  // обязательное
    OnDeleteAction on_delete = OnDeleteAction::NO_ACTION;
};
```

Добавить `ForeignKeyConstraint` в `Constraint` variant.

### 1.3 `sql/parser.cpp` — `parse_constraint()`

Разобрать inline-синтаксис:

```sql
col_name TYPE REFERENCES ref_table(ref_col) [ON DELETE {CASCADE | SET NULL | RESTRICT | NO ACTION}]
```

Логика:
1. Если текущий токен — `REFERENCES`, начать парсинг FK
2. Следующий — `IDENTIFIER` (имя referenced table)
3. `(` → `IDENTIFIER` (referenced column) → `)`
4. Опционально: `ON DELETE` → `CASCADE` | `SET NULL` | `RESTRICT` | `NO ACTION`

---

## Phase 2 — Meta Types & Conversion

### 2.1 `types/include/meta_column.hpp`

```cpp
enum class FkOnDeleteAction : uint8_t { NO_ACTION = 0, RESTRICT = 1, CASCADE = 2, SET_NULL = 3 };

struct MetaForeignKeyConstraint {
    UUID referenced_table_id;
    ColumnId referenced_column_id;
    FkOnDeleteAction on_delete = FkOnDeleteAction::NO_ACTION;
};
```

### 2.2 `misc/convert.cpp` — `convert(const Constraint&)`

Добавить ветку для `ForeignKeyConstraint`:

```cpp
if (const auto* fk = std::get_if<ForeignKeyConstraint>(&constraint)) {
    MetaForeignKeyConstraint mfk;
    mfk.on_delete = static_cast<FkOnDeleteAction>(fk->on_delete);
    return mfk;
}
```

Имена referenced table/column нужны на стадии анализа — добавить временные строковые поля
`ref_table_name` и `ref_col_name` в `MetaForeignKeyConstraint` (не персистируются,
заполняются из `ForeignKeyConstraint::referenced_table/column.value` в `convert()`).

---

## Phase 3 — Serialization (`storage/std_storage_serializer.cpp`)

Добавить тег `3` для `MetaForeignKeyConstraint`.

**Write:**
```
[uint8_t tag = 3]
[16 bytes: referenced_table_id UUID]
[16 bytes: referenced_column_id UUID]
[uint8_t: on_delete action]
```

**Read (case 3):**
```cpp
MetaForeignKeyConstraint fk;
stream.read_uuid(fk.referenced_table_id);
stream.read_uuid(fk.referenced_column_id);
uint8_t action; stream.read(&action, 1);
fk.on_delete = static_cast<FkOnDeleteAction>(action);
out.constraints.emplace_back(std::move(fk));
```

---

## Phase 4 — Semantic Analysis (`executor/semantic_analyzer.cpp`)

### 4.1 `analyze_create_table()` — разрешение UUID-ов

После `MetaColumn::MetaColumn(def)` для каждой колонки с `MetaForeignKeyConstraint`:
1. Найти referenced table по имени → `db_.get_table(ref_table_name, schema_name)`
2. Найти referenced column по имени
3. Проверить, что у referenced column есть PRIMARY KEY или UNIQUE индекс
4. Записать `referenced_table_id` и `referenced_column_id` в `MetaForeignKeyConstraint`

Ошибка, если таблица или колонка не существует.

### 4.2 `analyze_drop_table()` — защита от orphan FK

Перед удалением таблицы: пройти по всем таблицам, найти FK-constraints, указывающие
на неё → вернуть ошибку. Это предотвращает битые ссылки.

### 4.3 `analyze_insert()` / `analyze_update()`

Здесь — только типовая совместимость (уже есть). Проверка существования значения
в referenced table — **runtime**, делается в executor.

---

## Phase 5 — Executor FK Enforcement

### 5.1 Вспомогательная функция

```cpp
// Проверяет, что value существует в колонке ref_col таблицы ref_table
bool fk_value_exists(IDbInstance& db, const MetaTable& ref_table,
                     const ColumnId& ref_col_id, const DataToken& value);
```

Реализация: seq_scan по referenced table с поиском значения.
Если есть индекс на referenced column — использовать index_scan.

### 5.2 `InsertNodeExecutor::next()`

После формирования строки, перед вставкой:
```
for each column with MetaForeignKeyConstraint:
    if value is NOT NULL:
        resolve referenced MetaTable by referenced_table_id
        if !fk_value_exists(db, ref_table, ref_col_id, value):
            throw EngineException("FK violation: ...")
```

### 5.3 `UpdateNodeExecutor::next()`

Для каждого assignment: если обновляемая колонка имеет FK — проверить новое значение
через `fk_value_exists`.

### 5.4 `DeleteNodeExecutor::next()` — основная сложность

После получения строк для удаления:

```
for each deleted_row:
    for each table in db.get_all_tables():
        for each column with MetaForeignKeyConstraint pointing to current table:
            child_rows = seq_scan(child_table) where fk_col == deleted_value
            switch on_delete:
                CASCADE      → db_.delete_rows(child_rows, txn)   // рекурсивно
                SET_NULL     → db_.update_row(child_rows, col=NULL, txn)
                RESTRICT /
                NO_ACTION    → if !child_rows.empty(): throw error
```

Рекурсия CASCADE: `delete_rows` уже идёт через WAL и транзакцию, рекурсия
разворачивается естественно. Для защиты от циклических FK добавить ограничение
глубины рекурсии (например, `depth > 32 → throw`).

---

## Phase 6 — WAL / Recovery

Все вторичные операции (CASCADE delete, SET NULL update) идут через существующие
`db_.delete_rows()` / `db_.update_row()`, которые уже пишут в WAL. При UNDO recovery
они откатятся вместе с родительской транзакцией.

**Дополнительных изменений в WAL/recovery не требуется.**

---

## Порядок реализации

| # | Что | Файлы |
|---|-----|-------|
| 1 | Keywords + AST types | `dictionary.hpp`, `ast_tree.hpp`, `sql_token.hpp` |
| 2 | Parser: parse REFERENCES clause | `parser.cpp` |
| 3 | Meta types + FkOnDeleteAction | `meta_column.hpp` |
| 4 | convert() для ForeignKeyConstraint | `convert.cpp` |
| 5 | Serializer: case 3 | `std_storage_serializer.cpp` |
| 6 | Analyzer: UUID resolution в CREATE TABLE | `semantic_analyzer.cpp` |
| 7 | Analyzer: DROP TABLE защита | `semantic_analyzer.cpp` |
| 8 | InsertNodeExecutor: FK check | `node_executor.cpp` |
| 9 | UpdateNodeExecutor: FK check | `node_executor.cpp` |
| 10 | DeleteNodeExecutor: CASCADE / SET NULL | `node_executor.cpp` |

---

## Что не входит в v1

- `ON UPDATE CASCADE / SET NULL` — только ON DELETE
- Table-level `FOREIGN KEY (col)` синтаксис — только inline на колонке
- `ALTER TABLE ADD FOREIGN KEY`
- Граф FK в каталоге (обратные ссылки) — пока seq_scan по всем таблицам
- `information_schema.referential_constraints`
