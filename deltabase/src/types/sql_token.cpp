//
// Created by poproshaikin on 13.11.25.
//

#include "include/sql_token.hpp"

namespace types
{
    bool
    SqlToken::is_literal() const
    {
        return type == SqlTokenType::LITERAL;
    }
}