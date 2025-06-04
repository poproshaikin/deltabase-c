#include "include/token.hpp"

using namespace sql;

SqlToken::SqlToken(std::string lexeme, SqlTokenType type, SqlTokenValue value) {
    this->lexeme = lexeme;
    this->type = type;
    this->value = value;
}
