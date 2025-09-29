#pragma once

#include "../../sql/include/parser.hpp"
#include "../../storage/include/storage.hpp"


namespace converter {

    auto
    convert_create_table_to_mt(const sql::CreateTableStatement& stmt) -> storage::meta_table;

    auto
    create_row_update(const storage::meta_table& table, const sql::UpdateStatement& query) -> storage::data_row_update;

    auto
    convert_insert_to_data_row(const storage::meta_table& table, const sql::InsertStatement& insert) -> storage::data_row;

} // namespace converter