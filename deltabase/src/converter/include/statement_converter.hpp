#pragma once

#include "../../sql/include/parser.hpp"
#include "../../storage/include/storage.hpp"


namespace converter {

    auto
    convert_create_table_to_mt(const sql::CreateTableStatement& stmt) -> storage::MetaTable;

    auto
    create_row_update(const storage::MetaTable& table, const sql::UpdateStatement& query) -> storage::DataRowUpdate;

    auto
    convert_insert_to_data_row(const storage::MetaTable& table, const sql::InsertStatement& insert) -> storage::DataRow;

} // namespace converter