#include <cstring>
#include <stdexcept>
#include "../sql/include/lexer.hpp"
#include "../sql/include/parser.hpp"
#include "../misc/include/exceptions.hpp"
#include "../misc/include/utils.hpp"
#include "../meta/include/meta_factory.hpp"
#include "../executor/include/literals.hpp"
#include "include/converter.hpp"

extern "C" {
    #include "../core/include/data.h"
}

namespace converter {
    std::pair<void *, size_t> convert_str_to_literal(const std::string &literal,
                                                   DataType expected_type) {
        void *data = nullptr;
        size_t size = 0;

        if (expected_type == DT_INTEGER) {
            int32_t val = exe::literal_to_int(literal);
            size = sizeof(val);
            data = malloc(size);
            memcpy(data, &val, size);
        } else if (expected_type == DT_REAL) {
            double val = exe::literal_to_real(literal);
            size = sizeof(val);
            data = malloc(size);
            memcpy(data, &val, size);
        } else if (expected_type == DT_CHAR) {
            char val = exe::literal_to_char(literal);
            size = sizeof(val);
            data = malloc(size);
            memcpy(data, &val, size);
        } else if (expected_type == DT_BOOL) {
            bool val = exe::literal_to_bool(literal);
            size = sizeof(val);
            data = malloc(size);
            memcpy(data, &val, size);
        } else if (expected_type == DT_STRING) {
            const char *str = literal.c_str();
            size = literal.size();
            data = malloc(size);
            memcpy(data, str, size);
        }

        return {data, size};
    }

    DataToken *convert_astnode_to_token(const sql::AstNode *node,
                                        DataType expected_type) {
        const sql::SqlToken &token = std::get<sql::SqlToken>(node->value);
        std::string literal = token.value;

        const auto value = convert_str_to_literal(literal, expected_type);
        if (!value.first)
            throw std::runtime_error(
                "Unsupported DataType in convert_astnode_to_token");

        return make_token(expected_type, value.first, value.second);
    }

    FilterOp parse_filter_op(sql::AstOperator op) {
        if (op == sql::AstOperator::EQ)
            return OP_EQ;
        if (op == sql::AstOperator::NEQ)
            return OP_NEQ;
        if (op == sql::AstOperator::LT)
            return OP_LT;
        if (op == sql::AstOperator::LTE)
            return OP_LTE;
        if (op == sql::AstOperator::GR)
            return OP_GT;
        if (op == sql::AstOperator::GRE)
            return OP_GTE;
        throw std::runtime_error("Unknown filter operator");
    }

    DataFilter convert_binary_to_filter(const sql::BinaryExpr &where,
                                        const MetaTable &table) {
        DataFilter filter = {};

        if (where.op == sql::AstOperator::AND ||
            where.op == sql::AstOperator::OR) {
            const auto &left_expr =
                std::get<sql::BinaryExpr>(where.left->value);
            const auto &right_expr =
                std::get<sql::BinaryExpr>(where.right->value);

            DataFilter *left_filter =
                new DataFilter(convert_binary_to_filter(left_expr, table));
            DataFilter *right_filter =
                new DataFilter(convert_binary_to_filter(right_expr, table));

            filter.is_node = true;
            filter.data.node.left = left_filter;
            filter.data.node.right = right_filter;
            filter.data.node.op =
                (where.op == sql::AstOperator::AND) ? LOGIC_AND : LOGIC_OR;
            return filter;
        }

        const auto &left = std::get<sql::SqlToken>(where.left->value);
        const auto &right = std::get<sql::SqlToken>(where.right->value);
        const char *column_name = left.value.data();

        MetaColumn *column = find_column(column_name, &table);
        if (!column) {
            throw std::runtime_error("Column doesn't exist");
        }

        const auto value = convert_str_to_literal(right.value, column->data_type);

        filter.is_node = false;
        memcpy(filter.data.condition.column_id, column->column_id,
               sizeof(uuid_t));
        filter.data.condition.op = parse_filter_op(where.op);
        filter.data.condition.type = column->data_type;
        filter.data.condition.value = value.first;
        return filter;
    }

    MetaColumn convert_def_to_mc(const sql::ColumnDefinition &definition) {
        return meta::init_meta_column(
            definition.name.value, 
            convert_kw_to_dt(definition.type.get_detail<sql::SqlKeyword>()),
            convert_tokens_to_cfs(definition.constraints)
        );
    }

    std::vector<MetaColumn> convert_defs_to_mcs(std::vector<sql::ColumnDefinition> defs) {
        std::vector<MetaColumn> mcs;
        mcs.reserve(defs.size());

        for (int i = 0; i < defs.size(); i++) {
            mcs.push_back(convert_def_to_mc(defs[i]));
        }

        return mcs;
    }

    DataType convert_kw_to_dt(const sql::SqlKeyword& kw) {
        switch (kw) {
            case sql::SqlKeyword::_NULL:   return DT_NULL;
            case sql::SqlKeyword::INTEGER: return DT_INTEGER;
            case sql::SqlKeyword::STRING:  return DT_STRING;
            case sql::SqlKeyword::REAL:    return DT_REAL;
            case sql::SqlKeyword::CHAR:    return DT_CHAR;
            case sql::SqlKeyword::BOOL:    return DT_BOOL;
            default:                       return DT_UNDEFINED;
        }
    }

    DataColumnFlags convert_tokens_to_cfs(const std::vector<sql::SqlToken>& constraints) {
        DataColumnFlags flags = CF_NONE; 

        for (int i = 0; i < constraints.size(); i++) {
            if (!constraints[i].is_constraint()) {
                throw std::runtime_error("Passed invalid constraint token while converting to column flags");
            }

            sql::SqlKeyword kw;
            kw = constraints[i].get_detail<sql::SqlKeyword>();

            if (kw == sql::SqlKeyword::PRIMARY) {
                i++;
                if (i >= constraints.size() || constraints[i].get_detail<sql::SqlKeyword>() != sql::SqlKeyword::KEY) {
                    throw InvalidStatementSyntax();
                }
                flags = static_cast<DataColumnFlags>(flags | CF_PK);
            } 
            else if (kw == sql::SqlKeyword::NOT) {
                i++;
                if (i >= constraints.size() || constraints[i].get_detail<sql::SqlKeyword>() != sql::SqlKeyword::_NULL) {
                    throw InvalidStatementSyntax();
                }
                flags = static_cast<DataColumnFlags>(flags | CF_NN);
            }
            else if (kw == sql::SqlKeyword::AUTOINCREMENT) {
                flags = static_cast<DataColumnFlags>(flags | CF_AI);   
            }
            else if (kw == sql::SqlKeyword::UNIQUE) {
                flags = static_cast<DataColumnFlags>(flags | CF_UN);
            }
        }

        return flags;
    }
}