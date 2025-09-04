#pragma once

#include "meta_object.hpp"
#include "../../engine/include/config.hpp"

#include <unordered_map>
#include <ctime>

namespace catalog {
    struct MetaRegistrySnapshotDiff {
        std::unordered_map<std::string, CppMetaTable> added_tables;
        std::unordered_map<std::string, CppMetaSchema> added_schemas;
        std::unordered_map<std::string, CppMetaColumn> added_columns;

        std::unordered_map<std::string, CppMetaTable> updated_tables;
        std::unordered_map<std::string, CppMetaSchema> updated_schemas;
        std::unordered_map<std::string, CppMetaColumn> updated_columns;

        std::unordered_map<std::string, CppMetaTable> removed_tables;
        std::unordered_map<std::string, CppMetaSchema> removed_schemas;
        std::unordered_map<std::string, CppMetaColumn> removed_columns;

    private:
        MetaRegistrySnapshotDiff() = default;
        friend class MetaRegistrySnapshot;
    };

    struct MetaRegistrySnapshot {

        std::unordered_map<std::string, CppMetaTable> tables;
        std::unordered_map<std::string, CppMetaSchema> schemas;
        std::unordered_map<std::string, CppMetaColumn> columns;

        std::time_t captured_time;

        MetaRegistrySnapshot() = default;
        MetaRegistrySnapshot(
            std::unordered_map<std::string, CppMetaTable> tables,
            std::unordered_map<std::string, CppMetaSchema> schemas,
            std::unordered_map<std::string, CppMetaColumn> columns
        );

        bool operator==(const MetaRegistrySnapshot& other) const;

        MetaRegistrySnapshotDiff
        diff(const MetaRegistrySnapshot& other) const;
    };

    class MetaRegistry {
        MetaRegistrySnapshot initial_snapshot_;

        std::unordered_map<std::string, CppMetaTable> tables_;
        std::unordered_map<std::string, CppMetaSchema> schemas_;
        std::unordered_map<std::string, CppMetaColumn> columns_;

        engine::EngineConfig cfg_;

        void upload();
        void download();

        MetaRegistrySnapshot
        make_snapshot() const;

        void 
        upload_table(const CppMetaTable& table);
        void 
        upload_schema(const CppMetaSchema& schema);

    public:
        MetaRegistry(engine::EngineConfig cfg);
        void
        save_changes();
        void
        save_changes(std::string db_name);
        ~MetaRegistry();

        bool
        has_table(const std::string& table_name) const noexcept; 
        bool
        has_table(const std::string& table_name, const std::string& schema_name) const;
        bool
        has_column(const std::string& column_name, const std::string& table_name) const;
        bool
        has_column(const std::string& column_name, const std::string& table_name, const std::string& schema_name) const;
        bool 
        has_schema(const std::string& schema_name) const noexcept;

        const CppMetaTable&
        get_table(const std::string& table_name) const;
        const CppMetaTable&
        get_table(const std::string& table_name, const std::string& schema_name) const;
        const CppMetaColumn&
        get_column(const std::string& column_name, const std::string& table_name) const;
        const CppMetaColumn&
        get_column(const std::string& column_name, const std::string& table_name, const std::string& schema_name) const;
        const CppMetaSchema& 
        get_schema(const std::string& schema_name) const;

        void
        add_table(const CppMetaTable& table);
        void 
        add_schema(const CppMetaSchema& schema);
        void 
        add_column(const CppMetaColumn& column);

        void 
        update_table(const CppMetaTable& updated);
        void
        update_column(const CppMetaColumn& updated);
        void
        update_schema(const CppMetaSchema& updated);

        void
        remove_table(const CppMetaTable& table);
        void
        remove_column(const CppMetaColumn& column);
        void 
        remove_schema(const CppMetaSchema& schema);
    };
}