#include "../catalog/include/meta_registry.hpp"
#include "../sql/include/parser.hpp"
#include <cstdlib>
#include <stdexcept>

#include "include/converter.hpp"
#include "include/statement_converter.hpp"

extern "C" {
#include "../core/include/meta.h"
#include "../core/include/misc.h"
}

namespace converter {
    DataRowUpdate
    create_row_update(const MetaTable& table, const sql::UpdateStatement& query) {
        size_t assignments_count = query.assignments.size();

        uuid_t* indices = (uuid_t*)std::malloc(assignments_count * sizeof(uuid_t));
        void** values = (void**)std::malloc(assignments_count * sizeof(void*));

        for (size_t i = 0; i < assignments_count; i++) {
            const sql::BinaryExpr& assignment =
                std::get<sql::BinaryExpr>(query.assignments[i].value);
            const sql::SqlToken& left = std::get<sql::SqlToken>(assignment.left->value);
            const sql::SqlToken& right = std::get<sql::SqlToken>(assignment.right->value);

            MetaColumn* column = find_column(left.value.data(), &table);
            if (!column) {
                throw std::runtime_error("Column doesn't exist");
            }

            const auto value = convert_str_to_literal(right.value, column->data_type);

            values[i] = malloc(value.second);
            memcpy(indices[i], column->id, sizeof(uuid_t));
            memcpy(values[i], value.first, value.second);
        }

        DataRowUpdate update = {
            .count = assignments_count, .column_indices = indices, .values = values};

        return update;
    }

    DataRow
    convert_insert_to_data_row(const MetaTable& table, const sql::InsertStatement& insert) {
        DataRow row;
        row.flags = RF_NONE;
        row.count = table.columns_count;
        row.tokens = (DataToken**)calloc(row.count, sizeof(DataToken*));
        if (!row.tokens) {
            throw std::runtime_error("Failed to allocate tokens array");
        }

        for (uint64_t i = 0; i < table.columns_count; ++i) {
            const MetaColumn* col = table.columns[i];
            row.tokens[i] = nullptr;

            int insert_idx = -1;
            for (size_t j = 0; j < insert.columns.size(); ++j) {
                const char* ins_col_name = insert.columns[j].value.data();
                if (strcmp(ins_col_name, col->name) == 0) {
                    insert_idx = (int)j;
                    break;
                }
            }

            if (insert_idx == -1) {
                row.tokens[i] = make_token(DT_NULL, nullptr, 0);
            } else {
                const sql::SqlToken& value_token = insert.values[insert_idx];
                const auto value = convert_str_to_literal(value_token.value, col->data_type);
                row.tokens[i] = make_token(col->data_type, value.first, value.second);
            }
        }

        return row;
    }

    MetaTable
    convert_create_table_to_mt(const sql::CreateTableStatement& stmt) {
        return catalog::create_meta_table(stmt.name.value, stmt.columns);
    }
} // namespace converter