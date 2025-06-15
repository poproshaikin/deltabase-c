#include "include/lexer.hpp"
#include <cctype>
#include <iostream>
#include <stdexcept>
#include <sstream>

using namespace sql;

SqlToken::SqlToken(SqlTokenType type, std::string value, size_t line, size_t pos, SqlTokenDetail detail)
    : type(type), line(line), value(std::move(value)), pos(pos), detail(detail) { }

size_t get_word_length(const char* str) {
    const char* p = str;

    while (*p != '\0' && (isalnum(*p) || *p == '_')) {
        ++p;
    }

    return p - str;
}

size_t get_number_length(const char* str) {
    const char* p = str;
    bool dot_found = false;

    while (isdigit(*p) || (!dot_found && *p == '.')) {
        if (*p == '.') dot_found = true;
        ++p;
    }

    return p - str;
}

std::string read_next_word(const std::string& sql, size_t i) {
    size_t word_start = i;
    size_t word_len = get_word_length(&sql[i]);

    return sql.substr(word_start, word_len);
}

bool starts_as_operator(char c) {
    auto& operators = getOperatorsMap();

    for (const auto& op : operators) {
        if (op.first[0] == c) {
            return true;
        }
    }

    return false;
}

std::vector<SqlToken> SqlTokenizer::tokenize(const std::string& sql) {
    auto isalpha = [](char c){
        return std::isalpha((unsigned char)c) || (unsigned char)c == '_';
    };
    auto isdigit = [](char c){
        return std::isdigit((unsigned char)c);
    };
    auto isspace = [](char c){
        return std::isspace((unsigned char)c);
    };

    const auto& keywords = getKeywordsMap();
    const auto& operators = getOperatorsMap();
    const auto& symbols = getSymbolsMap();

    size_t line = 1;
    size_t pos = 0;

    std::vector<SqlToken> result;

    size_t i = 0;
    while (i < sql.length()) {
        char c = sql[i];

        std::cout << std::to_string(i) << std::endl;

        if (std::isspace((unsigned char) c)) {
            if (c == '\n') {
                line++;
                pos = 0;
            } else {
                pos++;
            }
            i++;
            continue;
        } 
        else if (isalpha(c)) {
            std::string word = read_next_word(sql, i);
            SqlToken token(SqlTokenType::IDENTIFIER, word, line, pos);

            auto it = keywords.find(word);
            if (it != keywords.end()) {
                token.type = SqlTokenType::KEYWORD;
                token.detail = it->second;
            }

            result.push_back(token);

            pos += word.length();
            i += word.length();
        }
        else if (c == '\'') {
            size_t word_start = i + 1;
            size_t word_end = sql.find('\'', word_start);

            if (word_end == std::string::npos) {
                throw std::runtime_error("Unterminated string literal");
            }

            std::string word = sql.substr(word_start, word_end - word_start);
            SqlToken token(SqlTokenType::LITERAL, word, line, pos, SqlLiteral::STRING);

            result.push_back(token);

            size_t consumed = (word_end + 1) - i;
            i += consumed;
            pos += consumed;
        }
        else if (isdigit(c)) {
            size_t word_start = i;
            size_t word_len = get_number_length(&sql[i]);

            std::string word = sql.substr(word_start, word_len);

            SqlToken token(SqlTokenType::LITERAL, word, line, pos);

            if (word.find('.') != std::string::npos || word.find('e') != std::string::npos || word.find('E') != std::string::npos) {
                token.detail = SqlLiteral::REAL;
            } else {
                token.detail = SqlLiteral::INTEGER;
            }
            
            result.push_back(token);

            i += word_len;
            pos += word_len;
        }
        else if (starts_as_operator(c)) {
            std::string op;
            size_t op_start = i;

            size_t max_len = 3;
            while (max_len > 0 && op_start + max_len <= sql.length()) {
                std::string candidate = sql.substr(op_start, max_len);
                if (operators.find(candidate) != operators.end()) {
                    op = candidate;
                    break;
                }
                max_len--;
            }

            if (op.empty()) {
                throw std::runtime_error("Unknown operator at position " + std::to_string(pos));
            }

            SqlToken token(SqlTokenType::OPERATOR, op, line, pos, operators.find(op)->second);
            result.push_back(token);

            i += op.length();
            pos += op.length();
        }
        else if (symbols.find(std::string(1, c)) != symbols.end()) {
            std::string sym_str(1, c);
            SqlToken token(SqlTokenType::SYMBOL, sym_str, line, pos, symbols.at(sym_str));
            result.push_back(token);

            i++;
            pos++;
        }
    }

    return result;
}

std::string SqlToken::to_string() const {
    std::ostringstream result;
    result << "Token:\n";
    result << "    Type: " << utils::to_string(type) << "\n";
    result << "    Value: " << value << "\n";
    result << "    Line: " << line << "\n";
    result << "    Pos: " << pos << "\n";

    switch (type) {
        case SqlTokenType::KEYWORD:
            result << "    Keyword: " << utils::to_string(std::get<SqlKeyword>(detail)) << "\n";
            break;
        case SqlTokenType::OPERATOR:
            result << "    Operator: " << utils::to_string(std::get<SqlOperator>(detail)) << "\n";
            break;
        case SqlTokenType::LITERAL:
            result << "    Literal: " << utils::to_string(std::get<SqlLiteral>(detail), value) << "\n";
            break;
        case SqlTokenType::SYMBOL:
            result << "    Symbol: " << utils::to_string(std::get<SqlSymbol>(detail)) << "\n";
            break;
        default:
            break;
    }

    return result.str();
}
