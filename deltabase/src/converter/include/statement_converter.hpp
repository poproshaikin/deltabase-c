#include "../../sql/include/parser.hpp"

extern "C" {
    #include "../../core/include/meta.h"
    #include "../../core/include/data.h"
    #include "../../core/include/misc.h"
}

namespace converter {
    MetaTable convert_create_table_to_mt(const sql::CreateTableStatement& stmt);

    DataRowUpdate create_row_update(const MetaTable& table,
                                    const sql::UpdateStatement &query);

    DataRow convert_insert_to_data_row(const MetaTable& table,
                                         const sql::InsertStatement &insert);

}