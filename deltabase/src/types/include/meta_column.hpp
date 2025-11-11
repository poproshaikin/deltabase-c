//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_META_COLUMN_HPP
#define DELTABASE_META_COLUMN_HPP
#include "typedefs.hpp"
#include "value_type.hpp"

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
        std::string id;
        std::string table_id;
        std::string name;
        ValueType type;
        MetaColumnFlags flags;

        explicit
        MetaColumn();

        explicit
        MetaColumn(const sql::ColumnDefinition& def);

        // Restrict copying
        MetaColumn(const MetaColumn&) = delete;

        MetaColumn(MetaColumn&&) = default;

        MetaColumn(const std::string& name, ValueType type, MetaColumnFlags flags);

        static bool
        try_deserialize(const Bytes& bytes, MetaColumn& out);
        Bytes
        serialize() const;
    };
}

#endif //DELTABASE_META_COLUMN_HPP