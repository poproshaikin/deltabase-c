#pragma once

#include "../../sql/include/parser.hpp"
#include "../../storage/include/storage.hpp"


namespace converter {

    storage::MetaTable
    convert_create_table_to_mt(const sql::CreateTableStatement& stmt);

    storage::DataRowUpdate
    create_row_update(const storage::MetaTable& table, const sql::UpdateStatement& query);

    storage::DataRow
    convert_insert_to_data_row(const storage::MetaTable& table, const sql::InsertStatement& insert);

} // namespace converter