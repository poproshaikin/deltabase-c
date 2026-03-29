//
// Created by poproshaikin on 11.11.25.
//

#ifndef DELTABASE_META_SCHEMA_HPP
#define DELTABASE_META_SCHEMA_HPP
#include "uuid.hpp"

#include <string>

namespace types
{
    using SchemaId = Uuid;

    struct MetaSchema
    {
        SchemaId id;
        std::string name;
        std::string db_name;

    public:
        bool operator==(const MetaSchema& other) const = default;
        bool operator!=(const MetaSchema& other) const = default;
    };
}

#endif //DELTABASE_META_SCHEMA_HPP