#pragma once 

#include <cstdint>

namespace storage {
    enum class ValueType : uint64_t {
        _NULL = 0,
        UNDEFINED = 0,
        INTEGER = 1,
        REAL,
        CHAR,
        BOOL,
        STRING
    };
}