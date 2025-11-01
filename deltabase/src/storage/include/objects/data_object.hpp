#pragma once

#include <memory>
#include <type_traits>
#include <variant>

#include "../shared.hpp"
#include "meta_object.hpp"

namespace storage
{
    using assignment = std::pair<MetaColumn, unique_void_ptr>;

    enum class DataRowFlags
    {
        NONE = 0,
        OBSOLETE = 1 << 0
    };

    DataRowFlags
    operator|(DataRowFlags left, DataRowFlags right)
    {
        using T = std::underlying_type_t<DataRowFlags>;
        return static_cast<DataRowFlags>(static_cast<T>(left) | static_cast<T>(right));
    }

    DataRowFlags
    operator|=(DataRowFlags& left, DataRowFlags right)
    {
        using T = std::underlying_type_t<DataRowFlags>;
        auto value = static_cast<DataRowFlags>(static_cast<T>(left) | static_cast<T>(right));
        left = value;
        return value;
    }

    enum class FilterOp
    {
        EQ = 1,
        NEQ,
        LT,
        LTE,
        GT,
        GTE
    };

    enum class LogicOp
    {
        AND = 1,
        OR
    };

    struct DataToken {
        bytes_v bytes;
        ValueType type;

        DataToken(bytes_v bytes, ValueType type);

        uint64_t
        estimate_size() const;

        bytes_v
        serialize() const;
    };

    uint64_t
    get_type_size(ValueType type);

    using RowId = uint64_t;

    struct DataRowUpdate
    {
        MetaTable table;
        std::vector<assignment> assignments;
    };

    struct DataRow
    {
        // initialize only in the storage
        uint64_t row_id;

        DataRowFlags flags;
        std::vector<DataToken> tokens;

        uint64_t
        estimate_size() const;

        bytes_v 
        serialize() const;

        DataRow 
        update(const DataRowUpdate& update) const;
    };

    struct DataTable
    {
        MetaTable table;
        std::vector<DataRow> rows;
    };

    struct DataFilterCondition
    {
        std::string column_id;
        FilterOp op;
        ValueType type;
        bytes_v data;
    };

    struct DataFilter;

    struct DataFilterNode
    {
        std::unique_ptr<DataFilter> left;
        LogicOp op;
        std::unique_ptr<DataFilter> right;
    };

    struct DataFilter
    {
        std::variant<DataFilterCondition, DataFilterNode> value;
    };
} // namespace storage