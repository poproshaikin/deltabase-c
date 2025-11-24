//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_VALUE_TYPE_HPP
#define DELTABASE_VALUE_TYPE_HPP

#include <cstdint>

namespace types
{
    enum class DataType : uint64_t
    {
        _NULL = 0,
        UNDEFINED = 0,
        INTEGER = 1,
        REAL,
        CHAR,
        BOOL,
        STRING
    };
}

#endif //DELTABASE_VALUE_TYPE_HPP