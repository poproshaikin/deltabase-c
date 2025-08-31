#include "include/lexer.hpp"
#include "../converter/include/converter.hpp"

namespace sql::utils {

    std::string
    to_string(sql::SqlTokenType type) {
        switch (type) {
        case SqlTokenType::IDENTIFIER:
            return "IDENTIFIER";
        case SqlTokenType::KEYWORD:
            return "KEYWORD";
        case SqlTokenType::OPERATOR:
            return "OPERATOR";
        case SqlTokenType::LITERAL:
            return "LITERAL";
        case SqlTokenType::SYMBOL:
            return "SYMBOL";
        default:
            return "UNKNOWN";
        }
    }

    std::string
    to_string(sql::SqlKeyword type) {
        const auto& keywords = keywords_map();
        for (const auto& [key, value] : keywords) {
            if (value == type) {
                return key;
            }
        }
        return {};
    }

    std::string
    to_string(sql::SqlOperator type) {
        const auto& operators = operators_map();
        for (const auto& [key, value] : operators) {
            if (value == type) {
                return key;
            }
        }
        return {};
    }

    std::string
    to_string(sql::SqlSymbol type) {
        const auto& symbols = symbols_map();
        for (const auto& [key, value] : symbols) {
            if (value == type) {
                return key;
            }
        }
        return {};
    }

    std::string
    to_string(sql::SqlLiteral type, const std::string& value) {
        std::string literal;

        switch (type) {
        case sql::SqlLiteral::INTEGER:
            literal = "INTEGER";
            break;
        case sql::SqlLiteral::STRING:
            literal = "STRING";
            break;
        case sql::SqlLiteral::BOOL:
            literal = "BOOL";
            break;
        case sql::SqlLiteral::CHAR:
            literal = "CHAR";
            break;
        case sql::SqlLiteral::REAL:
            literal = "REAL";
            break;
        default:
            literal = "UNKNOWN";
            break;
        }

        return literal + ": " + value;
    }

    std::string
    get_data_type_str(DataType dt) {
        const auto& data_types = data_types_map();
        SqlKeyword kw = converter::get_data_type_kw(dt);

        for (const auto& kvp : data_types) {
            if (kvp.second == kw) {
                return kvp.first;
            }
        }

        throw std::runtime_error("Failed to get data type str");
    }
}