//
// Created by poproshaikin on 6/7/26.
//

#ifndef DELTABASE_META_SEQUENCE_HPP
#define DELTABASE_META_SEQUENCE_HPP
#include "UUID.hpp"

#include <cstdint>

namespace types
{
    struct MetaSequence
    {
        UUID id;
        UUID schema_id;
        std::string name;
        std::string schema_name; // runtime-only, not serialized

        int32_t current_value;
    };
}

#endif //DELTABASE_META_SEQUENCE_HPP
