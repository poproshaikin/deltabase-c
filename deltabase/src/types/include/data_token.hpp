//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_DATA_TOKEN_HPP
#define DELTABASE_DATA_TOKEN_HPP

#include "sql_token.hpp"
#include "typedefs.hpp"
#include "value_type.hpp"

#include <cstdint>

namespace types
{
    struct DataToken
    {
        Bytes bytes;
        ValueType type;

        explicit
        DataToken(const SqlToken& sql_token);

        explicit
        DataToken(const Bytes& bytes, ValueType type);

        uint64_t
        estimate_size() const;

        Bytes
        serialize() const;
    };

}

#endif //DELTABASE_DATA_TOKEN_HPP