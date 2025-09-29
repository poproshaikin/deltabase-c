#pragma once

#include "objects/meta_object.hpp"
#include "pages/page.hpp"
#include "shared.hpp"
#include <filesystem>

namespace storage {
    namespace fs = std::filesystem;

    class file_manager {
        fs::path data_dir_;

        void write_file(const fs::path& path, const bytes_v& content);

        std::optional<fs::path>
        find_page_path(const std::string& db_name, const std::string& page_id) const;
        std::optional<fs::path>
        find_table_path(const std::string& db_name, const std::string& table_id) const;
        std::optional<fs::path>
        find_schema_path(const std::string& db_name, const std::string& schema_id) const;

    public:
        explicit file_manager(const fs::path& data_dir);

        bytes_v read_file(const fs::path& path) const;

        bool
        page_exists(const std::string& db_name, const std::string& page_id) const;
        bool
        exists_page(
            const std::string& db_name,
            const std::string& schema_name,
            const std::string& table_name,
            const std::string& page_id
        ) const;

        data_page
        load_page(const std::string& db_name, const std::string& page_id) const;

        data_page
        load_page(
            const std::string& db_name,
            const std::string& schema_name,
            const std::string& table_name,
            const std::string& page_id
        ) const;

        void
        save_page(
            const std::string& db_name,
            const std::string& schema_name,
            const std::string& table_name,
            const data_page& page
        );

        bool
        table_exists(const std::string& db_name, const std::string& table_id) const;

        bool
        table_exists(
            const std::string& db_name,
            const std::string& schema_name,
            const std::string& table_name
        ) const;

        meta_table
        load_table(const std::string& db_name, const std::string& table_id) const;

        meta_table
        load_table(
            const std::string& db_name,
            const std::string& schema_name,
            const std::string& table_name
        ) const;

        void
        save_table(
            const std::string& db_name, const std::string& schema_name, const meta_table& table
        );

        
        bool
        schema_exists_by_id(const std::string& db_name, const std::string& schema_id) const;
        bool
        schema_exists_by_name(const std::string& db_name, const std::string& schema_name) const;

        
        meta_schema
        load_schema_by_id(const std::string& db_name, const std::string& schema_id) const;
        meta_schema
        load_schema_by_name(const std::string& db_name, const std::string& schema_name) const;

        void
        save_schema(const meta_schema& schema);

        std::vector<fs::path>
        get_tables_paths(const std::string& db_name, const std::string& schema_name);

        std::vector<fs::path>
        get_schemata_paths(const std::string& db_name);

        bool
        create_dir(const fs::path& path);
    };
}