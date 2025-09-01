#pragma once 

#include "data_object.hpp"
#include "meta_object.hpp"

namespace catalog::information_schema {

    auto
    get_tables_schema() -> CppMetaTable;

    auto
    get_tables_data(const std::vector<CppMetaTable>& tables) -> CppDataTable;

    auto 
    get_columns_schema() -> CppMetaTable;

    auto
    get_columns_data(const std::vector<CppMetaColumn>& columns) -> CppDataTable;
}