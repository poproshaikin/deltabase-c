//
// Created by poproshaikin on 02.12.25.
//

#include "meta_column.hpp"

#include "../misc/include/convert.hpp"

namespace types
{
    MetaColumn::MetaColumn(const std::string& name, DataType type, const std::vector<ColumnConstraint>& constraints)
        : name(name), type(type), constraints(constraints)
    {
    }

}