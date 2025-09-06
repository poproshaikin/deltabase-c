#include "include/models.hpp"

#include "../misc/include/utils.hpp"
#include <algorithm>

namespace catalog {
    
    static DataType convert_kw_to_dt(const sql::SqlKeyword& kw) {
        switch (kw) {
        case sql::SqlKeyword::_NULL:
            return DT_NULL;
        case sql::SqlKeyword::INTEGER:
            return DT_INTEGER;
        case sql::SqlKeyword::STRING:
            return DT_STRING;
        case sql::SqlKeyword::REAL:
            return DT_REAL;
        case sql::SqlKeyword::CHAR:
            return DT_CHAR;
        case sql::SqlKeyword::BOOL:
            return DT_BOOL;
        default:
            return DT_UNDEFINED;
        }
    }

    static DataColumnFlags convert_tokens_to_cfs(const std::vector<sql::SqlToken>& constraints) {
        DataColumnFlags flags = CF_NONE;

        for (size_t i = 0; i < constraints.size(); i++) {
            if (!constraints[i].is_constraint()) {
                continue;
            }

            sql::SqlKeyword kw = constraints[i].get_detail<sql::SqlKeyword>();

            if (kw == sql::SqlKeyword::PRIMARY) {
                i++;
                if (i >= constraints.size() ||
                    constraints[i].get_detail<sql::SqlKeyword>() != sql::SqlKeyword::KEY) {
                    continue;
                }
                flags = static_cast<DataColumnFlags>(flags | CF_PK);
            } else if (kw == sql::SqlKeyword::NOT) {
                i++;
                if (i >= constraints.size() ||
                    constraints[i].get_detail<sql::SqlKeyword>() != sql::SqlKeyword::_NULL) {
                    continue;
                }
                flags = static_cast<DataColumnFlags>(flags | CF_NN);
            } else if (kw == sql::SqlKeyword::AUTOINCREMENT) {
                flags = static_cast<DataColumnFlags>(flags | CF_AI);
            } else if (kw == sql::SqlKeyword::UNIQUE) {
                flags = static_cast<DataColumnFlags>(flags | CF_UN);
            }
        }

        return flags;
    }

    static MetaColumn convert_def_to_mc(const sql::ColumnDefinition& definition) {
        MetaColumn column;
        
        column.name = make_c_string(definition.name.value);
        column.data_type = convert_kw_to_dt(definition.type.get_detail<sql::SqlKeyword>());
        column.flags = convert_tokens_to_cfs(definition.constraints);
        uuid_generate_time(column.id);
        
        return column;
    }

    static std::vector<MetaColumn> convert_defs_to_mcs(const std::vector<sql::ColumnDefinition>& defs) {
        std::vector<MetaColumn> mcs;
        mcs.reserve(defs.size());

        for (const auto& def : defs) {
            mcs.push_back(convert_def_to_mc(def));
        }

        return mcs;
    }

    auto
    create_meta_column(const std::string& name, DataType type, DataColumnFlags flags) -> MetaColumn {
        MetaColumn column;

        column.name = make_c_string(name);
        column.data_type = type;
        column.flags = flags;
        uuid_generate_time(column.id);

        return column;
    }

    auto
    create_meta_table(const std::string& name, const std::vector<sql::ColumnDefinition> col_defs) -> MetaTable {
        MetaTable table;

        table.name = make_c_string(name);
        
        auto columns = convert_defs_to_mcs(col_defs);
        table.columns = (MetaColumn*)malloc(columns.size() * sizeof(MetaColumn));
        std::copy(columns.begin(), columns.end(), table.columns);
        
        table.columns_count = col_defs.size();
        table.has_pk = false;
        table.last_rid = 0;
        uuid_generate_time(table.id);

        return table;
    }

    auto
    create_meta_schema(const std::string& name) -> MetaSchema {
        MetaSchema schema;
        schema.name = make_c_string(name);
        uuid_generate_time(schema.id);
        return schema;
    }
}
