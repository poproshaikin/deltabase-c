#pragma once

#include <string>
#include <vector>

#include "../../sql/include/parser.hpp"

extern "C" {
#include "../../core/include/meta.h"
}

namespace catalog {

    MetaColumn
    create_meta_column(const std::string& name, DataType type, DataColumnFlags flags);

    MetaTable
    create_meta_table(const std::string& name, const std::vector<sql::ColumnDefinition> col_defs);
}