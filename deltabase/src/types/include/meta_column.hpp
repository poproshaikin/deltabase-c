//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_META_COLUMN_HPP
#define DELTABASE_META_COLUMN_HPP
#include "ast_tree.hpp"
#include "typedefs.hpp"
#include "uuid.hpp"
#include "data_type.hpp"

#include <string>

namespace types
{
    enum class MetaColumnFlags
    {
        NONE = 0,
        PK = 1 << 0,
        FK = 1 << 1,
        AI = 1 << 2,
        NN = 1 << 3,
        UN = 1 << 4
    };

    inline MetaColumnFlags
    operator|(MetaColumnFlags left, MetaColumnFlags right)
    {
        using T = std::underlying_type_t<MetaColumnFlags>;
        return static_cast<MetaColumnFlags>(static_cast<T>(left) | static_cast<T>(right));
    }

    struct MetaColumn
    {
        Uuid id;
        Uuid table_id;
        std::string name;
        DataType type;
        MetaColumnFlags flags;

        explicit
        MetaColumn();
        explicit
        MetaColumn(const ColumnDefinition& def);
        explicit
        MetaColumn(const std::string& name, DataType type, MetaColumnFlags flags);
    };
}

#endif //DELTABASE_META_COLUMN_HPP