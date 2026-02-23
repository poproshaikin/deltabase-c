//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_LEXER_HPP
#define DELTABASE_LEXER_HPP
#include "../../types/include/sql_token.hpp"

#include <string>
#include <vector>

namespace sql
{
    std::vector<types::SqlToken>
    lex(const std::string& query);
}

#endif //DELTABASE_LEXER_HPP