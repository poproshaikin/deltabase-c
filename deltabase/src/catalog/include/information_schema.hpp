#pragma once 

#include "data_object.hpp"
#include "meta_schema.hpp"

namespace catalog::information_schema {

    CppMetaTable
    get_tables_schema();

    CppDataTable
    get_tables_data(const std::vector<CppMetaTable>& tables);

    CppMetaTable 
    get_columns_schema();

    CppDataTable
    get_columns_data(const std::vector<CppMetaColumn>& columns);
}