#pragma once 

#include <string>
namespace exe {
    int literal_to_int(const std::string& literal);
    bool literal_to_bool(const std::string& literal);
    char literal_to_char(const std::string& literal);
    double literal_to_real(const std::string& literal);
}
