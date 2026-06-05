//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_META_COLUMN_HPP
#define DELTABASE_META_COLUMN_HPP
#include "UUID.hpp"
#include "ast_tree.hpp"
#include "data_type.hpp"
#include "typedefs.hpp"
#include "data_token.hpp"

#include <optional>
#include <vector>

#include <string>

namespace types
{
    using ColumnId = UUID;

    struct MetaNotNullConstraint {};

    struct MetaDefaultConstraint
    {
        DataToken value;
    };

    struct MetaAutoIncrementConstraint {};

    struct MetaForeignKeyConstraint
    {
        UUID referenced_table_id;
        ColumnId referenced_column_id;
        // if referenced ids are not known at parse time, they can be left null/zero
    };

    using ColumnConstraint = std::variant<
        MetaNotNullConstraint,
        MetaDefaultConstraint,
        MetaAutoIncrementConstraint,
        MetaForeignKeyConstraint
    >;

    struct MetaColumn
    {
        ColumnId id;
        UUID table_id;
        std::string name;
        DataType type;
        std::vector<ColumnConstraint> constraints;

        explicit
        MetaColumn() = default;

        explicit
        MetaColumn(const ColumnDefinition& def);

        explicit
        MetaColumn(const std::string& name, DataType type, const std::vector<ColumnConstraint>& constraints);

        template <typename T>
        bool
        has_constraint() const
        {
            for (const auto& c : constraints)
                if (std::holds_alternative<T>(c)) return true;
            return false;
        }

        template <typename T>
        const T*
        get_constraint() const
        {
            for (const auto& c : constraints)
                if (const T* p = std::get_if<T>(&c)) return p;
            return nullptr;
        }

        template <typename T>
        T*
        get_constraint()
        {
            for (auto& c : constraints)
                if (T* p = std::get_if<T>(&c)) return p;
            return nullptr;
        }
    };
}

#endif //DELTABASE_META_COLUMN_HPP