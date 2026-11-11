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

    DataRowFlags inline
    operator|(DataRowFlags left, DataRowFlags right)
    {
        using T = std::underlying_type_t<DataRowFlags>;
        return static_cast<DataRowFlags>(static_cast<T>(left) | static_cast<T>(right));
    }

    DataRowFlags inline
    operator|=(DataRowFlags& left, DataRowFlags right)
    {
        using T = std::underlying_type_t<DataRowFlags>;
        const auto value = static_cast<DataRowFlags>(static_cast<T>(left) | static_cast<T>(right));
        left = value;
        return value;
    }

    DataRowFlags
    inline
    operator&(DataRowFlags left, DataRowFlags right)
    {
        using T = std::underlying_type_t<DataRowFlags>;
        return static_cast<DataRowFlags>(
            static_cast<T>(left) & static_cast<T>(right)
        );
    }

    DataRowFlags inline
    operator&=(DataRowFlags& left, const DataRowFlags right)
    {
        return left = left & right;
    }

    inline DataRowFlags
    operator~(DataRowFlags flags)
    {
        using T = std::underlying_type_t<DataRowFlags>;
        return static_cast<DataRowFlags>(~static_cast<T>(flags));
    }

    using RowId = uint64_t;

    struct DataRow
    {
        RowId id{};
        DataRowFlags flags{};
        std::vector<DataToken> tokens;

        DataRow() = default;
        explicit DataRow(const std::vector<SqlToken>& sql_tokens);
    };

    // Approximate in-memory size of a row, for the per-query memory budget. Distinct from
    // IIOManager::estimate_size, which measures the serialized size and leaves out the
    // container overhead -- several times smaller for narrow rows.
    inline uint64_t
    memory_footprint(const DataRow& row)
    {
        uint64_t size = sizeof(DataRow) + row.tokens.capacity() * sizeof(DataToken);

        for (const auto& token : row.tokens)
            size += token.bytes.capacity();

        return size;
    }
} // namespace types

#endif // DELTABASE_DATA_ROW_HPP