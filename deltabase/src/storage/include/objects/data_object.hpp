#pragma once

#include <memory>
#include <type_traits>
#include <variant>

#include "../shared.hpp"
#include "meta_object.hpp"

namespace storage {
    using assignment = std::pair<meta_column, unique_void_ptr>;

    enum class data_row_flags {
        NONE = 0,
        OBSOLETE = 1 << 0
    };

    data_row_flags operator|(data_row_flags left, data_row_flags right) {
        using T = std::underlying_type_t<data_row_flags>;
        return static_cast<data_row_flags>(static_cast<T>(left) | static_cast<T>(right));
    }      

    data_row_flags operator|=(data_row_flags& left, data_row_flags right) {
        using T = std::underlying_type_t<data_row_flags>;
        auto value = static_cast<data_row_flags>(static_cast<T>(left) | static_cast<T>(right));
        left = value;
        return value;
    } 

    enum class filter_op {
        EQ = 1,
        NEQ,
        LT,
        LTE,
        GT,
        GTE
    };

    enum class logic_op {
        AND = 1,
        OR 
    };

    struct data_token {
        bytes_v bytes;
        ValueType type;

        data_token(bytes_v bytes, ValueType type);

        uint64_t
        estimate_size() const;

        bytes_v
        serialize() const;
    };

    using RowId = uint64_t;

    struct data_row_update {
        meta_table table;
        std::vector<assignment> assignments;
    };

    struct data_row {
        // initialize only in the storage
        uint64_t row_id;
        data_row_flags flags;
        std::vector<data_token> tokens;  

        uint64_t
        estimate_size() const;

        bytes_v 
        serialize() const;

        data_row 
        update(const data_row_update& update) const;
    };

    struct data_table {
        meta_table table;
        std::vector<data_row> rows;
    };

    struct data_filter_condition {
        std::string column_id;
        filter_op op;
        ValueType type;
        bytes_v data;
    };

    struct data_filter;

    struct data_filter_node {
        std::unique_ptr<data_filter> left;
        logic_op op;
        std::unique_ptr<data_filter> right; 
    };

    struct data_filter {
        std::variant<data_filter_condition, data_filter_node> value;
    };
}