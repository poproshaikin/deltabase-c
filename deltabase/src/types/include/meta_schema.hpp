//
// Created by poproshaikin on 11.11.25.
//

#ifndef DELTABASE_META_SCHEMA_HPP
#define DELTABASE_META_SCHEMA_HPP
#include "uuid.hpp"

#include <string>

namespace types
{
    class MetaSchema
    {
        Uuid id;
        std::string name;
        std::string db_name;
    };
}

#endif //DELTABASE_META_SCHEMA_HPP