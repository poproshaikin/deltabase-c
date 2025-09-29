#pragma once

#include <string>
#include <utility>

#include "../../sql/include/parser.hpp"
#include "../../storage/include/storage.hpp"

namespace converter {
    std::vector<std::byte>
    convert_str_to_literal(const std::string& literal, storage::ValueType expected_type);

    auto
    convert_astnode_to_token(const sql::AstNode* node, storage::ValueType expected_type) -> storage::data_token;

    auto
    parse_filter_op(sql::AstOperator op) -> storage::filter_op;

    auto
    convert_binary_to_filter(const sql::BinaryExpr& where, const storage::meta_table& table) -> storage::data_filter;

    auto
    convert_defs_to_mcs(std::vector<sql::ColumnDefinition> defs) -> std::vector<storage::meta_column>;

    auto
    convert_kw_to_vt(const sql::SqlKeyword& kw) -> storage::ValueType;

    auto
    convert_tokens_to_cfs(const std::vector<sql::SqlToken>& constraints) -> storage::meta_column_flags;

    auto
    get_data_type_kw(storage::ValueType dt) -> sql::SqlKeyword;
} // namespace converter