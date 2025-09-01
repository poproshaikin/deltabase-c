#include "include/literals.hpp"
#include <algorithm>
#include <stdexcept>

namespace exe {
    auto
    literal_to_int(const std::string& literal) -> int {
        try {
            size_t pos;
            int val = std::stoi(literal, &pos);
            if (pos != literal.length()) {
                throw std::invalid_argument("Extra characters after number");
            }
            return val;
        } catch (const std::exception& e) {
            throw std::runtime_error("Invalid integer format: " + literal);
        }
    }

    auto
    literal_to_real(const std::string& literal) -> double {
        try {
            size_t pos;
            double val = std::stod(literal, &pos);
            if (pos != literal.length()) {
                throw std::invalid_argument("Extra characters after number");
            }
            return val;
        } catch (const std::exception& e) {
            throw std::runtime_error("Invalid double format: " + literal);
        }
    }

    auto
    literal_to_bool(const std::string& literal) -> bool {
        std::string lower;
        lower.resize(literal.size());
        std::ranges::transform(literal, lower.begin(), ::tolower);

        if (lower == "true" || lower == "1" || lower == "t")
            return true;
        if (lower == "false" || lower == "0" || lower == "f")
            return false;

        throw std::runtime_error("Invalid boolean format: " + literal);
    }

    auto
    literal_to_char(const std::string& literal) -> char {
        if (literal.size() != 1) {
            throw std::runtime_error("Invalid char format (must be single character): " + literal);
        }
        return literal[0];
    }
} // namespace exe
