#include "include/lexer.hpp"

std::string sql::utils::to_string(sql::SqlTokenType type) {
    switch (type) {
        case SqlTokenType::IDENTIFIER: return "IDENTIFIER";
        case SqlTokenType::KEYWORD:    return "KEYWORD";
        case SqlTokenType::OPERATOR:   return "OPERATOR";
        case SqlTokenType::LITERAL:    return "LITERAL";
        case SqlTokenType::SYMBOL:     return "SYMBOL";
        default:                       return "UNKNOWN";
    }
}
std::string sql::utils::to_string(sql::SqlKeyword type) {
    const auto& keywords = getKeywordsMap();
    for (const auto& [key, value] : keywords) {
        if (value == type) {
            return key;
        }
    }
    return {}; 
}
std::string sql::utils::to_string(sql::SqlOperator type) {
    const auto& operators = getOperatorsMap();
    for (const auto& [key, value] : operators) {
        if (value == type) {
            return key;
        }
    }
    return {}; 
}
std::string sql::utils::to_string(sql::SqlSymbol type) {
    const auto& symbols = getSymbolsMap();
    for (const auto& [key, value] : symbols) {
        if (value == type) {
            return key;
        }
    }
    return {}; 
}
std::string sql::utils::to_string(sql::SqlLiteral type, const std::string& value) {
    std::string literal;

    switch (type) {
        case sql::SqlLiteral::INTEGER: literal = "INTEGER"; break;
        case sql::SqlLiteral::STRING:  literal = "STRING"; break;
        case sql::SqlLiteral::BOOL:    literal = "BOOL"; break;
        case sql::SqlLiteral::CHAR:    literal = "CHAR"; break;
        case sql::SqlLiteral::REAL:    literal = "REAL"; break;
        default:                       literal = "UNKNOWN"; break;
    }
    
    return literal + ": " + value;
}
