#include <utility>
#include <string>

#include "../../sql/include/parser.hpp"

extern "C" {
    #include "../../core/include/meta.h"
    #include "../../core/include/data.h"
}

namespace converter {
    std::pair<void *, std::size_t> convert_str_to_literal(const std::string& literal, DataType expected_type);

    DataToken *convert_astnode_to_token(const sql::AstNode *node, DataType expected_type);

    FilterOp parse_filter_op(sql::AstOperator op);

    DataFilter convert_binary_to_filter(const sql::BinaryExpr &where,
                             const MetaTable &table);

    MetaColumn convert_def_to_mc(const sql::ColumnDefinition &def);

    std::vector<MetaColumn> convert_defs_to_mcs(std::vector<sql::ColumnDefinition> defs);

    DataType convert_kw_to_dt(const sql::SqlKeyword& kw);

    DataColumnFlags convert_tokens_to_cfs(const std::vector<sql::SqlToken>& constraints);
} // namespace exe::converter