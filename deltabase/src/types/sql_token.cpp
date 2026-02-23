//
// Created by poproshaikin on 13.11.25.
//

#include "include/sql_token.hpp"
#include "../sql/include/dictionary.hpp"

namespace types
{
    SqlToken::SqlToken(SqlTokenType type,
                      std::string value,
                      size_t line,
                      size_t position,
                      SqlTokenDetail detail)
        : type(type), value(std::move(value)), line(line), pos(position), detail(detail)
    {
    }

    bool
    SqlToken::is_data_type() const
    {
        if (!is_keyword())
            return false;
        
        auto kw = get_detail<SqlKeyword>();
        return sql::is_data_type_kw(kw);
    }

    bool
    SqlToken::is_constraint() const
    {
        if (!is_keyword())
            return false;
        
        auto kw = get_detail<SqlKeyword>();
        return kw == SqlKeyword::PRIMARY || 
               kw == SqlKeyword::UNIQUE || 
               kw == SqlKeyword::NOT || 
               kw == SqlKeyword::_NULL ||
               kw == SqlKeyword::AUTOINCREMENT;
    }

    bool
    SqlToken::is_keyword() const
    {
        return type == SqlTokenType::KEYWORD;
    }

    bool
    SqlToken::is_literal() const
    {
        return type == SqlTokenType::LITERAL;
    }

    bool
    SqlToken::is_identifier() const
    {
        return type == SqlTokenType::IDENTIFIER;
    }

    std::string
    SqlToken::to_string(int indent) const
    {
        std::string result;
        std::string indent_str(indent, ' ');
        
        result += indent_str + "SqlToken {\n";
        result += indent_str + "  type: ";
        
        switch (type)
        {
        case SqlTokenType::KEYWORD:
            result += "KEYWORD";
            break;
        case SqlTokenType::IDENTIFIER:
            result += "IDENTIFIER";
            break;
        case SqlTokenType::LITERAL:
            result += "LITERAL";
            break;
        case SqlTokenType::OPERATOR:
            result += "OPERATOR";
            break;
        case SqlTokenType::SYMBOL:
            result += "SYMBOL";
            break;
        default:
            result += "UNKNOWN";
            break;
        }
        
        result += "\n";
        result += indent_str + "  value: \"" + value + "\"\n";
        result += indent_str + "  line: " + std::to_string(line) + "\n";
        result += indent_str + "  pos: " + std::to_string(pos) + "\n";
        result += indent_str + "}";
        
        return result;
    }
}
