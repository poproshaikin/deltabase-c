//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_DATA_ROW_UPDATE_HPP
#define DELTABASE_DATA_ROW_UPDATE_HPP
#include "meta_table.hpp"
#include "typedefs.hpp"

#include <utility>

namespace types
{
    using Assignment = std::pair<MetaColumn, Bytes>;

    struct DataRowUpdate
    {
        const MetaTable& table;
        std::vector<Assignment> assignments;
    };
}

#endif //DELTABASE_DATA_ROW_UPDATE_HPP