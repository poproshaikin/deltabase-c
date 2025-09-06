#pragma once

#include <string>
#include <utility>

#include "../../sql/include/parser.hpp"
#include "../../catalog/include/data_object.hpp"

extern "C" {
#include "../../core/include/data.h"
#include "../../core/include/meta.h"
}

namespace converter {
    auto
    convert_str_to_literal(const std::string& literal, DataType expected_type) -> std::pair<void*, std::size_t>;

    auto
    convert_astnode_to_token(const sql::AstNode* node, DataType expected_type) -> DataToken;

    auto
    parse_filter_op(sql::AstOperator op) -> FilterOp;

    auto
    convert_binary_to_filter(const sql::BinaryExpr& where, const catalog::CppMetaTable& table) -> catalog::CppDataFilter;

    auto
    convert_def_to_mc(const sql::ColumnDefinition& def) -> MetaColumn;

    auto
    convert_defs_to_mcs(std::vector<sql::ColumnDefinition> defs) -> std::vector<MetaColumn>;

    auto
    convert_kw_to_dt(const sql::SqlKeyword& kw) -> DataType;

    auto
    convert_tokens_to_cfs(const std::vector<sql::SqlToken>& constraints) -> DataColumnFlags;

    auto
    get_data_type_kw(DataType dt) -> sql::SqlKeyword;
} // namespace converter