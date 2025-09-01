#ifndef EXE_LITERALS_HPP
#define EXE_LITERALS_HPP

#include <string>

namespace exe {
    auto
    literal_to_int(const std::string& literal) -> int;
    auto
    literal_to_bool(const std::string& literal) -> bool;
    auto
    literal_to_char(const std::string& literal) -> char;
    auto
    literal_to_real(const std::string& literal) -> double;
} // namespace exe

#endif
