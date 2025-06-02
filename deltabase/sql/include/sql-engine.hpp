#pragma once

#include "lexer.hpp"
#include <string>
#include <variant>
#include <vector>

namespace sql {
    class SqlEngine {
        public:
            std::vector<SqlToken> tokenize(std::string command);
        private:
            Lexer _lexer;
    };
}
