//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_DATA_TOKEN_HPP
#define DELTABASE_DATA_TOKEN_HPP

#include "value_type.hpp"

#include <cstdint>

namespace types
{
    struct DataToken
    {
        bytes_v bytes;
        ValueType type;

        DataToken(bytes_v bytes, ValueType type);

        uint64_t
        estimate_size() const;

        bytes_v
        serialize() const;
    };

}

#endif //DELTABASE_DATA_TOKEN_HPP