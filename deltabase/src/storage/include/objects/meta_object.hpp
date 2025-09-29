#pragma once 

#include <cstdint>
#include <vector>
#include <string>

#include "../shared.hpp"
#include "../../../sql/include/parser.hpp"

namespace storage {
    enum class meta_column_flags {
        NONE = 0,
        PK = 1 << 0,
        FK = 1 << 1,
        AI = 1 << 2,
        NN = 1 << 3,
        UN = 1 << 4
    };

    inline meta_column_flags operator|(meta_column_flags left, meta_column_flags right) {
        using T = std::underlying_type_t<meta_column_flags>;
        return static_cast<meta_column_flags>(static_cast<T>(left) | static_cast<T>(right));
    }

    struct meta_schema {
        std::string id;
        std::string name;
        std::string db_name;

        bool
        compare_content(const meta_schema& other);

        meta_schema();
        meta_schema(const meta_schema&) = delete;
        meta_schema(meta_schema&&) = default;

        static bool
        can_deserialize(bytes_v bytes);
        static meta_schema
        deserialize(bytes_v bytes);
        bytes_v
        serialize() const;
    };

    struct meta_column {
        std::string id;
        std::string table_id;
        std::string name;

        ValueType type;
        meta_column_flags flags;

        explicit meta_column();
        explicit meta_column(const sql::ColumnDefinition& def);
        // Restrict copying
        meta_column(const meta_column&) = delete;
        meta_column(meta_column&&) = default;
        meta_column(const std::string& name, ValueType type, meta_column_flags flags);

        static bool
        can_deserialize(bytes_v bytes);
        static meta_column
        deserialize(bytes_v bytes);
        bytes_v
        serialize() const;
    };

    struct meta_table {
        std::string id;
        std::string schema_id;
        std::string name;

        std::vector<meta_column> columns;
        uint64_t last_rid;

        meta_table();
        meta_table(meta_table&& other) = default;
        // Restrict copying 
        meta_table(const meta_table& other) = delete;

        bool
        has_column(const std::string& name) const;
        const meta_column&
        get_column(const std::string& name) const;
        
        bool
        compare_content(const meta_table& other) const;

        static bool
        can_deserialize(bytes_v bytes);
        static meta_table
        deserialize(bytes_v bytes);
        bytes_v
        serialize() const;
    };
}