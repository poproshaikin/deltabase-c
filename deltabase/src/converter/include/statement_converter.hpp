#pragma once

#include "../../sql/include/parser.hpp"

extern "C" {
#include "../../core/include/data.h"
#include "../../core/include/meta.h"
#include "../../core/include/misc.h"
}

namespace converter {

    auto
    convert_create_table_to_mt(const sql::CreateTableStatement& stmt) -> MetaTable;

    auto
    create_row_update(const MetaTable& table, const sql::UpdateStatement& query) -> DataRowUpdate;

    auto
    convert_insert_to_data_row(const MetaTable& table, const sql::InsertStatement& insert) -> DataRow;

} // namespace converter