//
// Created by poproshaikin on 12.11.25.
//
#include "include/lexer.hpp"
#include <sstream>

namespace sql
{
    using namespace types;

    std::string
    static
    to_lower(const std::string& str)
    {
        std::stringstream sb;
        for (char c : str)
            sb << static_cast<char>(std::tolower(c));

        return sb.str();
    }

    size_t
    static
    get_word_length(const char* str)
    {
        const char* p = str;

        while (*p != '\0' && (isalnum(*p) || *p == '_'))
            ++p;

        return p - str;
    }

    size_t
    static
    get_number_length(const char* str)
    {
        const char* p = str;
        bool dot_found = false;

        while (isdigit(*p) || (!dot_found && *p == '.'))
        {
            if (*p == '.')
                dot_found = true;
            ++p;
        }

        return p - str;
    }

    std::string
    static
    read_next_word(const std::string& sql, size_t i)
    {
        size_t word_start = i;
        size_t word_len = get_word_length(&sql[i]);

        return sql.substr(word_start, word_len);
    }

    bool
    static
    starts_as_operator(char c)
    {
        auto& operators = types::operators_map();

        for (const auto& op : operators)
            if (op.first[0] == c)
                return true;

        return false;
    }

    std::vector<SqlToken>
    lex(const std::string& query)
    {
        auto isalpha = [](char c)
        {
            return std::isalpha(
                       static_cast<unsigned char>(c)) || static_cast<unsigned char>(c) == '_';
        };
        auto isdigit = [](char c)
        {
            return std::isdigit(static_cast<unsigned char>(c));
        };
        auto isspace = [](char c)
        {
            return std::isspace(static_cast<unsigned char>(c));
        };

        const auto& keywords = keywords_map();
        const auto& operators = operators_map();
        const auto& symbols = symbols_map();

        size_t line = 1;
        size_t pos = 0;

        std::vector<SqlToken> result;

        size_t i = 0;
        while (i < query.length())
        {
            char c = query[i];

            if (isspace(c))
            {
                if (c == '\n')
                {
                    line++;
                    pos = 0;
                }
                else
                {
                    pos++;
                }
                i++;
            }
            else if (isalpha(c))
            {
                std::string word = to_lower(read_next_word(query, i));
                SqlToken token(SqlTokenType::IDENTIFIER, word, line, pos);

                auto it = keywords.find(word);
                if (it != keywords.end())
                {
                    token.type = SqlTokenType::KEYWORD;
                    token.detail = it->second;
                }

                result.push_back(token);

                pos += word.length();
                i += word.length();
            }
            else if (c == '\'')
            {
                size_t word_start = i + 1;
                size_t word_end = query.find('\'', word_start);

                if (word_end == std::string::npos)
                {
                    throw std::runtime_error("Unterminated string literal");
                }

                std::string word = query.substr(word_start, word_end - word_start);
                SqlToken token(SqlTokenType::LITERAL, word, line, pos, SqlLiteral::STRING);

                result.push_back(token);

                size_t consumed = (word_end + 1) - i;
                i += consumed;
                pos += consumed;
            }
            else if (isdigit(c))
            {
                size_t word_start = i;
                size_t word_len = get_number_length(&query[i]);

                std::string word = query.substr(word_start, word_len);

                SqlToken token(SqlTokenType::LITERAL, word, line, pos);

                if (word.find('.') != std::string::npos || word.find('e') != std::string::npos ||
                    word.find('E') != std::string::npos)
                {
                    token.detail = SqlLiteral::REAL;
                }
                else
                {
                    token.detail = SqlLiteral::INTEGER;
                }

                result.push_back(token);

                i += word_len;
                pos += word_len;
            }
            else if (starts_as_operator(c))
            {
                std::string op;
                size_t op_start = i;

                size_t max_len = 3;
                while (max_len > 0 && op_start + max_len <= query.length())
                {
                    std::string candidate = to_lower(query.substr(op_start, max_len));
                    if (operators.find(candidate) != operators.end())
                    {
                        op = candidate;
                        break;
                    }
                    max_len--;
                }

                if (op.empty())
                {
                    throw std::runtime_error("Unknown operator at position " + std::to_string(pos));
                }

                SqlToken token(SqlTokenType::OPERATOR, op, line, pos, operators.find(op)->second);
                result.push_back(token);

                i += op.length();
                pos += op.length();
            }
            else if (symbols.contains(std::string(1, c)))
            {
                std::string sym_str(1, c);
                SqlToken token(SqlTokenType::SYMBOL, sym_str, line, pos, symbols.at(sym_str));
                result.push_back(token);

                i++;
                pos++;
            }
            else if (c == '.')
            {
                std::string sym_str(1, c);
                SqlToken token(SqlTokenType::SYMBOL, sym_str, line, pos, SqlSymbol::PERIOD);
                result.push_back(token);

                i++;
                pos++;
            }
        }

        return result;
    }
}