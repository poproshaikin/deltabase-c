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

    struct MetaSchema {
        std::string id;
        std::string name;
        std::string db_name;

        bool
        compare_content(const MetaSchema& other);

        MetaSchema();
        MetaSchema(const MetaSchema&) = delete;
        MetaSchema(MetaSchema&&) = default;

        static bool
        can_deserialize(bytes_v bytes);
        static MetaSchema
        deserialize(bytes_v bytes);
        bytes_v
        serialize() const;
    };

    struct MetaColumn {
        std::string id;
        std::string table_id;
        std::string name;

        ValueType type;
        meta_column_flags flags;

        explicit MetaColumn();
        explicit MetaColumn(const sql::ColumnDefinition& def);
        // Restrict copying
        MetaColumn(const MetaColumn&) = delete;
        MetaColumn(MetaColumn&&) = default;
        MetaColumn(const std::string& name, ValueType type, meta_column_flags flags);

        static bool
        can_deserialize(bytes_v bytes);
        static MetaColumn
        deserialize(bytes_v bytes);
        bytes_v
        serialize() const;
    };

    struct MetaTable {
        std::string id;
        std::string schema_id;
        std::string name;

        std::vector<MetaColumn> columns;
        uint64_t last_rid;

        MetaTable();
        MetaTable(MetaTable&& other) = default;
        // Restrict copying 
        MetaTable(const MetaTable& other) = delete;

        bool
        has_column(const std::string& name) const;
        const MetaColumn&
        get_column(const std::string& name) const;
        
        bool
        compare_content(const MetaTable& other) const;

        static bool
        can_deserialize(bytes_v bytes);
        static MetaTable
        deserialize(bytes_v bytes);
        bytes_v
        serialize() const;
    };
}