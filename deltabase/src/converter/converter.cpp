#include "include/converter.hpp"
#include "../executor/include/literals.hpp"
#include "../misc/include/exceptions.hpp"
#include "../sql/include/lexer.hpp"
#include "../sql/include/parser.hpp"
#include <cstring>
#include <stdexcept>

namespace converter {
    storage::bytes_v
    convert_str_to_literal(const std::string& literal, storage::ValueType expected_type) {
        storage::bytes_v bytes;

        if (expected_type == storage::ValueType::INTEGER) {
            int32_t val = exe::literal_to_int(literal);
            bytes.resize(sizeof(val));
            memcpy(bytes.data(), &val, sizeof(val));
        } 
        else if (expected_type == storage::ValueType::REAL) {
            double val = exe::literal_to_real(literal);
            bytes.resize(sizeof(val));
            memcpy(bytes.data(), &val, sizeof(val));
        } else if (expected_type == storage::ValueType::CHAR) {
            char val = exe::literal_to_char(literal);
            bytes.resize(sizeof(val));
            memcpy(bytes.data(), &val, sizeof(val));
        } else if (expected_type == storage::ValueType::BOOL) {
            bool val = exe::literal_to_bool(literal);
            bytes.resize(sizeof(val));
            memcpy(bytes.data(), &val, sizeof(val));
        } else if (expected_type == storage::ValueType::STRING) {
            bytes.resize(literal.size());
            memcpy(bytes.data(), literal.data(), literal.size());
        }

        return bytes;
    }

    storage::DataToken
    convert_astnode_to_token(const sql::AstNode* node, storage::ValueType expected_type) {
        const auto& token = std::get<sql::SqlToken>(node->value);
        std::string literal = token.value;

        auto bytes = convert_str_to_literal(literal, expected_type);
        return storage::DataToken(bytes, expected_type);
    }

    storage::FilterOp
    parse_filter_op(sql::AstOperator op) {
        if (op == sql::AstOperator::EQ)
            return storage::FilterOp::EQ;
        if (op == sql::AstOperator::NEQ)
            return storage::FilterOp::NEQ;
        if (op == sql::AstOperator::LT)
            return storage::FilterOp::LT;
        if (op == sql::AstOperator::LTE)
            return storage::FilterOp::LTE;
        if (op == sql::AstOperator::GR)
            return storage::FilterOp::GT;
        if (op == sql::AstOperator::GRE)
            return storage::FilterOp::GTE;
        throw std::runtime_error("Unknown filter operator");
    }

    storage::DataFilter
    convert_binary_to_filter(const sql::BinaryExpr& where, const storage::MetaTable& table) {
        storage::DataFilter filter = {};

        if (where.op == sql::AstOperator::AND || where.op == sql::AstOperator::OR) {
            const auto& left_expr = std::get<sql::BinaryExpr>(where.left->value);
            const auto& right_expr = std::get<sql::BinaryExpr>(where.right->value);

            storage::DataFilter left_filter(convert_binary_to_filter(left_expr, table));
            storage::DataFilter right_filter(convert_binary_to_filter(right_expr, table));

            storage::DataFilterNode node;
            node.left = std::make_unique<storage::DataFilter>(std::move(left_filter));
            node.right = std::make_unique<storage::DataFilter>(std::move(right_filter));
            node.op = (where.op == sql::AstOperator::AND) ? storage::LogicOp::AND : storage::LogicOp::OR;

            filter.value = std::move(node);

            return filter;
        }

        const auto& left = std::get<sql::SqlToken>(where.left->value);
        const auto& right = std::get<sql::SqlToken>(where.right->value);
        const auto& column_name = left.value;

        if (!table.has_column(column_name)) {
            throw std::runtime_error("Column doesn't exist");
        }
        const storage::MetaColumn& column = table.get_column(column_name);
        storage::bytes_v literal = convert_str_to_literal(right.value, column.type);

        storage::DataFilterCondition condition;
        condition.op = parse_filter_op(where.op);
        condition.type = column.type;
        condition.data = literal;
        condition.column_id = column.id;

        filter.value = condition;

        return filter;
    }

    std::vector<storage::MetaColumn>
    convert_defs_to_mcs(std::vector<sql::ColumnDefinition> defs) {
        std::vector<storage::MetaColumn> mcs;
        mcs.reserve(defs.size());

        for (const auto & def : defs) {
            mcs.push_back(storage::MetaColumn(def));
        }

        return mcs;
    }

    storage::ValueType
    convert_kw_to_dt(const sql::SqlKeyword& kw) {
        switch (kw) {
        case sql::SqlKeyword::_NULL:
            return storage::ValueType::_NULL;
        case sql::SqlKeyword::INTEGER:
            return storage::ValueType::INTEGER;
        case sql::SqlKeyword::STRING:
            return storage::ValueType::STRING;
        case sql::SqlKeyword::REAL:
            return storage::ValueType::REAL;
        case sql::SqlKeyword::CHAR:
            return storage::ValueType::CHAR;
        case sql::SqlKeyword::BOOL:
            return storage::ValueType::BOOL;
        default:
            return storage::ValueType::UNDEFINED;
        }
    }

    storage::MetaColumnFlags
    convert_tokens_to_cfs(const std::vector<sql::SqlToken>& constraints) {
        storage::MetaColumnFlags flags = storage::MetaColumnFlags::NONE;

        for (int i = 0; i < constraints.size(); i++) {
            if (!constraints[i].is_constraint()) {
                throw std::runtime_error(
                    "Passed invalid constraint token while converting to column flags");
            }

            sql::SqlKeyword kw;
            kw = constraints[i].get_detail<sql::SqlKeyword>();

            if (kw == sql::SqlKeyword::PRIMARY) {
                i++;
                if (i >= constraints.size() ||
                    constraints[i].get_detail<sql::SqlKeyword>() != sql::SqlKeyword::KEY) {
                    throw InvalidStatementSyntax();
                }
                flags = static_cast<storage::MetaColumnFlags>(flags | storage::MetaColumnFlags::PK);
            } else if (kw == sql::SqlKeyword::NOT) {
                i++;
                if (i >= constraints.size() ||
                    constraints[i].get_detail<sql::SqlKeyword>() != sql::SqlKeyword::_NULL) {
                    throw InvalidStatementSyntax();
                }
                flags = static_cast<storage::MetaColumnFlags>(flags | storage::MetaColumnFlags::NN);
            } else if (kw == sql::SqlKeyword::AUTOINCREMENT) {
                flags = static_cast<storage::MetaColumnFlags>(flags | storage::MetaColumnFlags::AI);
            } else if (kw == sql::SqlKeyword::UNIQUE) {
                flags = static_cast<storage::MetaColumnFlags>(flags | storage::MetaColumnFlags::UN);
            }
        }

        return flags;
    }

    auto
    get_data_type_kw(storage::ValueType vt) -> sql::SqlKeyword {
        switch (vt) {
        case storage::ValueType::INTEGER: return sql::SqlKeyword::INTEGER; 
        case storage::ValueType::STRING:  return sql::SqlKeyword::STRING;  
        case storage::ValueType::BOOL:    return sql::SqlKeyword::BOOL;    
        case storage::ValueType::REAL:    return sql::SqlKeyword::REAL;    
        case storage::ValueType::CHAR:    return sql::SqlKeyword::CHAR;    
        case storage::ValueType::_NULL:   return sql::SqlKeyword::_NULL;   

        default:         throw std::runtime_error("This data type isn't supported");
        }
    }
} // namespace converter
