#pragma once

#include <string>

#include "../../sql/include/parser.hpp"
#include "../../storage/include/storage.hpp"

namespace converter {
    storage::bytes_v
    convert_str_to_literal(const std::string& literal, storage::ValueType expected_type);

    auto
    convert_astnode_to_token(const sql::AstNode* node, storage::ValueType expected_type) -> storage::DataToken;

    auto
    parse_filter_op(sql::AstOperator op) -> storage::FilterOp;

    auto
    convert_binary_to_filter(const sql::BinaryExpr& where, const storage::MetaTable& table) -> storage::DataFilter;

    auto
    convert_defs_to_mcs(std::vector<sql::ColumnDefinition> defs) -> std::vector<storage::MetaColumn>;

    auto
    convert_kw_to_vt(const sql::SqlKeyword& kw) -> storage::ValueType;

    auto
    convert_tokens_to_cfs(const std::vector<sql::SqlToken>& constraints) -> storage::MetaColumnFlags;

    auto
    get_data_type_kw(storage::ValueType dt) -> sql::SqlKeyword;
} // namespace converter