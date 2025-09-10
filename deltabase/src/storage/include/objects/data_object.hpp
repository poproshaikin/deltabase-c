#pragma once

#include <memory>
#include <variant>

#include "meta_object.hpp"

namespace storage {
    namespace detail {
        class FileDeleter {
            void operator()(void* ptr) {
                std::free(ptr);
            }
        };
    }

    using unique_void_ptr = std::unique_ptr<void, detail::FileDeleter>;
    using assignment = std::pair<MetaColumn, unique_void_ptr>;
    using literal = std::vector<std::byte>;

    enum class DataRowFlags {
        NONE = 0,
        OBSOLETE = 1 << 0
    };

    enum class FilterOp {
        EQ = 1,
        NEQ,
        LT,
        LTE,
        GT,
        GTE
    };

    enum class LogicOp {
        AND = 1,
        OR 
    };

    struct DataToken {
        literal bytes;
        ValueType type;

        DataToken(literal bytes, ValueType type);
    };

    struct DataRow {
        uint64_t row_id;
        DataRowFlags flags;
        std::vector<DataToken> tokens;  
    };

    struct DataTable {
        MetaTable table;
        std::vector<DataRow> rows;
    };

    struct DataFilterCondition {
        std::string column_id;
        FilterOp op;
        ValueType type;
        literal data;
    };

    struct DataFilter;

    struct DataFilterNode {
        std::unique_ptr<DataFilter> left;
        LogicOp op;
        std::unique_ptr<DataFilter> right; 
    };

    struct DataFilter {
        std::variant<DataFilterCondition, DataFilterNode> value;
    };

    struct DataRowUpdate {
        MetaTable table;
        std::vector<assignment> assignments;
    };
}