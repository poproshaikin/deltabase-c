#include "include/information_schema.hpp"
#include "../sql/include/lexer.hpp"

namespace catalog::information_schema {

    CppMetaTable
    get_tables_schema() {
        CppMetaTable result("tables", "information_schema");

        result.add_column(CppMetaColumn("id", DT_STRING, CF_NONE));
        result.add_column(CppMetaColumn("name", DT_STRING, CF_NONE));
        result.add_column(CppMetaColumn("last_rid", DT_INTEGER, CF_NONE));

        return result;
    }

    CppDataTable
    get_tables_data(const std::vector<CppMetaTable>& tables) {
        catalog::CppMetaTable schema = catalog::information_schema::get_tables_schema();
        catalog::CppDataTable result(schema);

        for (uint64_t i = 0; i < tables.size(); i++) {
            const auto& table = tables[i];
            std::string id_str = table.get_name() + "_schema";
            std::string table_name = table.get_name();
            std::string last_rid_str = "0"; // TODO: get actual last row id

            result.add_row(CppDataRow(i, { 
                CppDataToken(id_str.length(), const_cast<char*>(id_str.c_str()), DT_STRING),
                CppDataToken(table_name.length(), const_cast<char*>(table_name.c_str()), DT_STRING), 
                CppDataToken(last_rid_str.length(), const_cast<char*>(last_rid_str.c_str()), DT_STRING), 
            }));
        }
        
        return result;
    }

    CppMetaTable 
    get_columns_schema() {
        CppMetaTable result("columns", "information_schema");

        result.add_column(CppMetaColumn("id", DT_STRING, CF_NONE));
        result.add_column(CppMetaColumn("name", DT_STRING, CF_NONE));
        result.add_column(CppMetaColumn("data_type", DT_STRING, CF_NONE));

        return result;
    }

    CppDataTable
    get_columns_data(const std::vector<CppMetaColumn>& columns) {
        catalog::CppMetaTable schema = catalog::information_schema::get_columns_schema();
        catalog::CppDataTable result(schema);

        for (uint64_t i = 0; i < columns.size(); i++) {
            const auto& col = columns[i];
            std::string id_str = col.get_name() + "_schema";
            std::string col_name = col.get_name();
            std::string data_type = sql::utils::get_data_type_str(col.get_data_type());

            result.add_row(CppDataRow(i, {
                CppDataToken(id_str.length(), const_cast<char*>(id_str.c_str()), DT_STRING),
                CppDataToken(col_name.length(), const_cast<char*>(col_name.c_str()), DT_STRING),
                CppDataToken(data_type.length(), const_cast<char*>(data_type.c_str()), DT_STRING),
            }));
        }

        return result;
    }
}