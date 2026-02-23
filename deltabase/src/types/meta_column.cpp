//
// Created by poproshaikin on 02.12.25.
//

#include "meta_column.hpp"

#include "../misc/include/convert.hpp"

namespace types
{
    MetaColumn::MetaColumn(const std::string& name, DataType type, MetaColumnFlags flags)
        : name(name), type(type), flags(flags)
    {
    }

    MetaColumn::MetaColumn(const ColumnDefinition& def)
    {
        *this = misc::convert(def);
    }

}