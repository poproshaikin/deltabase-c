//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_DATA_ROW_HPP
#define DELTABASE_DATA_ROW_HPP
#include "data_row_update.hpp"
#include "data_token.hpp"

#include <cstdint>
#include <type_traits>
#include <vector>

namespace types
{
    enum class DataRowFlags
    {
        NONE = 0,
        OBSOLETE = 1 << 0
    };

    DataRowFlags
    inline
    operator|(DataRowFlags left, DataRowFlags right)
    {
        using T = std::underlying_type_t<DataRowFlags>;
        return static_cast<DataRowFlags>(static_cast<T>(left) | static_cast<T>(right));
    }

    DataRowFlags
    inline
    operator|=(DataRowFlags& left, DataRowFlags right)
    {
        using T = std::underlying_type_t<DataRowFlags>;
        const auto value = static_cast<DataRowFlags>(static_cast<T>(left) | static_cast<T>(right));
        left = value;
        return value;
    }

    using RowId = uint64_t;

    struct DataRow
    {
        // initialize only in the storage
        RowId row_id;
        DataRowFlags flags;
        std::vector<DataToken> tokens;

        uint64_t
        estimate_size() const;

        DataRow
        update(const DataRowUpdate& update) const;

        DataRow(const DataRow&) = delete;
        DataRow& operator=(const DataRow&) = delete;

        DataRow(DataRow&&) = default;
        DataRow& operator=(DataRow&&) = default;
    };
}

#endif //DELTABASE_DATA_ROW_HPP