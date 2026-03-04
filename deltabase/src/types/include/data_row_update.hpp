//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_DATA_ROW_UPDATE_HPP
#define DELTABASE_DATA_ROW_UPDATE_HPP
#include "data_token.hpp"
#include "typedefs.hpp"
#include "uuid.hpp"

#include <utility>

namespace types
{
    using ColumnId = Uuid;

    using AssignLiteral = std::pair<ColumnId, DataToken>;
    using AssignColumn = std::pair<ColumnId, ColumnId>;
    using Assignment = std::variant<AssignLiteral, AssignColumn>;

    using RowUpdate = std::vector<Assignment>;
}

#endif //DELTABASE_DATA_ROW_UPDATE_HPP