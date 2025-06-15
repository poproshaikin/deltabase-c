#include <iostream>
#include <string>
#include <vector>

#include "../src/sql/include/sql.hpp"

// int main() {
//     std::cout << "here";
// }

int main() {
    std::string sql = "SELECT * FROM users";

    sql::SqlTokenizer tokenizer;
    std::vector<sql::SqlToken> result = tokenizer.tokenize(sql);

    for (const auto& token : result) {
        std::cout << token.to_string();
    }
}
