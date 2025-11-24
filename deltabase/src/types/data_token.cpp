//
// Created by poproshaikin on 13.11.25.
//

#include "include/data_token.hpp"
#include "../misc/include/convert.hpp"

#include <assert.h>

namespace types
{
    DataToken::DataToken(const SqlToken& sql_token)
    {
        *this = misc::convert(sql_token);
    }

    DataToken::DataToken(const Bytes& bytes, DataType type)
        : bytes(bytes), type(type)
    {
    }
}