#include "include/converter.hpp"
#include "../executor/include/literals.hpp"
#include "../misc/include/exceptions.hpp"
#include "../sql/include/lexer.hpp"
#include "../sql/include/parser.hpp"
#include <cstring>
#include <stdexcept>
#include <algorithm>

extern "C" {
#include "../core/include/data.h"
}

namespace converter {
    auto
    convert_str_to_literal(const std::string& literal, DataType expected_type) -> std::pair<void*, size_t> {
        void* data = nullptr;
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
            const char* str = literal.c_str();
            size = literal.size();
            data = malloc(size);
            memcpy(data, str, size);
        }

        return {data, size};
    }

    auto
    convert_astnode_to_token(const sql::AstNode* node, DataType expected_type) -> DataToken {
        const auto& token = std::get<sql::SqlToken>(node->value);
        std::string literal = token.value;

        const auto value = convert_str_to_literal(literal, expected_type);
        if (!value.first)
            throw std::runtime_error("Unsupported DataType in convert_astnode_to_token");

        return make_token(expected_type, value.first, value.second);
    }

    auto
    parse_filter_op(sql::AstOperator op) -> FilterOp {
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

    auto
    convert_binary_to_filter(const sql::BinaryExpr& where, const catalog::CppMetaTable& table) -> catalog::CppDataFilter {
        if (where.op == sql::AstOperator::AND || where.op == sql::AstOperator::OR) {
            const auto& left_expr = std::get<sql::BinaryExpr>(where.left->value);
            const auto& right_expr = std::get<sql::BinaryExpr>(where.right->value);

            auto left_filter = std::make_shared<catalog::CppDataFilter>(convert_binary_to_filter(left_expr, table));
            auto right_filter = std::make_shared<catalog::CppDataFilter>(convert_binary_to_filter(right_expr, table));

            LogicOp logic_op = (where.op == sql::AstOperator::AND) ? LOGIC_AND : LOGIC_OR;
            auto node = catalog::CppDataFilterNode{left_filter, logic_op, right_filter};
            
            return catalog::CppDataFilter{node};
        }

        const auto& left = std::get<sql::SqlToken>(where.left->value);
        const auto& right = std::get<sql::SqlToken>(where.right->value);
        const std::string column_name = left.value;

        // Найдем колонку в таблице
        auto column_it = std::find_if(table.columns.begin(), table.columns.end(),
            [&column_name](const catalog::CppMetaColumn& col) {
                return col.name == column_name;
            });
        
        if (column_it == table.columns.end()) {
            throw std::runtime_error("Column doesn't exist");
        }

        const auto value = convert_str_to_literal(right.value, column_it->data_type);

        catalog::CppDataFilterCondition condition;
        condition.column_id = column_it->id;
        condition.operation = parse_filter_op(where.op);
        condition.type = column_it->data_type;
        condition.value = unique_void_ptr(value.first);

        catalog::CppDataFilter filter;
        filter.value = std::move(condition);
        return filter;
    }

    auto
    convert_def_to_mc(const sql::ColumnDefinition& definition) -> MetaColumn {
        // Создаем C++ объект
        catalog::CppMetaColumn cpp_column;
        cpp_column.id = ""; // Будет заполнен позже при добавлении в таблицу
        cpp_column.table_id = ""; // Будет заполнен позже
        cpp_column.name = definition.name.value;
        cpp_column.data_type = convert_kw_to_dt(definition.type.get_detail<sql::SqlKeyword>());
        cpp_column.flags = convert_tokens_to_cfs(definition.constraints);
        
        // Конвертируем в C структуру
        return cpp_column.to_c();
    }

    auto
    convert_defs_to_mcs(std::vector<sql::ColumnDefinition> defs) -> std::vector<MetaColumn> {
        std::vector<MetaColumn> mcs;
        mcs.reserve(defs.size());

        for (const auto & def : defs) {
            mcs.push_back(convert_def_to_mc(def));
        }

        return mcs;
    }

    auto
    convert_kw_to_dt(const sql::SqlKeyword& kw) -> DataType {
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

    auto
    convert_tokens_to_cfs(const std::vector<sql::SqlToken>& constraints) -> DataColumnFlags {
        DataColumnFlags flags = CF_NONE;

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
                flags = static_cast<DataColumnFlags>(flags | CF_PK);
            } else if (kw == sql::SqlKeyword::NOT) {
                i++;
                if (i >= constraints.size() ||
                    constraints[i].get_detail<sql::SqlKeyword>() != sql::SqlKeyword::_NULL) {
                    throw InvalidStatementSyntax();
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

    auto
    get_data_type_kw(DataType dt) -> sql::SqlKeyword {
        switch (dt) {
        case DT_INTEGER: return sql::SqlKeyword::INTEGER; 
        case DT_STRING:  return sql::SqlKeyword::STRING;  
        case DT_BOOL:    return sql::SqlKeyword::BOOL;    
        case DT_REAL:    return sql::SqlKeyword::REAL;    
        case DT_CHAR:    return sql::SqlKeyword::CHAR;    
        case DT_NULL:    return sql::SqlKeyword::_NULL;   

        default:         throw std::runtime_error("This data type isn't supported");
        }
    }
} // namespace converter
