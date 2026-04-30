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

    using ColumnConstraint = std::variant<
        MetaNotNullConstraint,
        MetaDefaultConstraint>;

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

        template <typename TConstraint>
        bool
        has_constraint() const
        {
            for (const auto& con : constraints)
            {
                if (std::holds_alternative<TConstraint>(con))
                {
                    return true;
                }
            }

            return false;
        }

        template <typename TConstraint>
        TConstraint*
        get_constraint()
        {
            for (auto& con : constraints)
            {
                if (std::holds_alternative<TConstraint>(con))
                {
                    return &std::get<TConstraint>(con);
                }
            }

            return nullptr;
        }
    };
}

#endif //DELTABASE_META_COLUMN_HPP