#pragma once

#include <string>
#include <vector>

extern "C" {
#include "../../core/include/meta.h"
}

namespace catalog {
    struct CppMetaSchema {
        std::string id;
        std::string name;
        std::string db_name;

        CppMetaSchema(std::string name, std::string db_name);

        [[nodiscard]] static CppMetaSchema
        from_c(const MetaSchema& c_schema);
        [[nodiscard]] MetaSchema
        to_c() const;

        static void
        cleanup_c(MetaSchema& schema);

        bool operator==(const CppMetaSchema& schema) const;
        bool operator!=(const CppMetaSchema& schema) const;

        bool
        compare_content(const CppMetaSchema& other) const;
    private:
        CppMetaSchema() = default;
    };

    struct CppMetaColumn {
        std::string id;
        std::string table_id;
        std::string name;

        DataType data_type;
        DataColumnFlags flags;

        CppMetaColumn() = default;
        CppMetaColumn(std::string name, DataType type, DataColumnFlags flags, std::string table_id);

        [[nodiscard]] static CppMetaColumn
        from_c(const MetaColumn& c_column);
        [[nodiscard]] MetaColumn
        to_c() const;

        static void
        cleanup_c(MetaColumn& schema);

        bool operator==(const CppMetaColumn& other) const;
        bool operator!=(const CppMetaColumn& other) const;

        bool
        compare_content(const CppMetaColumn& other) const;
    };

    struct CppMetaTable {
        std::string id;
        std::string schema_id;
        std::string name;

        std::vector<CppMetaColumn> columns;

        uint64_t last_rid;

        bool
        has_column(const std::string& name);

        [[nodiscard]] static CppMetaTable
        from_c(const MetaTable& c_table);
        [[nodiscard]] MetaTable
        to_c() const;

        static void
        cleanup_c(MetaTable& schema);

        bool operator==(const CppMetaTable& other) const;
        bool operator!=(const CppMetaTable& other) const;

        bool
        compare_content(const CppMetaTable& other) const;
    };
}