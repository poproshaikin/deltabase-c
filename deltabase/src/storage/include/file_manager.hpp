#pragma once

#include "objects/meta_object.hpp"
#include "pages/page.hpp"
#include "shared.hpp"
#include <cstdint>
#include <filesystem>
#include <fstream>

namespace storage {
    namespace fs = std::filesystem;

    class FileManager {
        fs::path data_dir_;

        void write_file(const fs::path& path, const bytes_v& content);

        std::optional<fs::path>
        find_page_path(const std::string& db_name, const std::string& page_id) const;
        std::optional<fs::path>
        find_table_path(const std::string& db_name, const std::string& table_id) const;
        std::optional<fs::path>
        find_schema_path(const std::string& db_name, const std::string& schema_id) const;

    public:
        explicit FileManager(const fs::path& data_dir);

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

        DataPage
        load_page(const std::string& db_name, const std::string& page_id) const;

        DataPage
        load_page(
            const std::string& db_name,
            const std::string& schema_name,
            const std::string& table_name,
            const std::string& page_id
        ) const;

        std::vector<DataPage>
        load_all_pages(const std::string& db_name) const;

        void
        save_page(
            const std::string& db_name,
            const std::string& schema_name,
            const std::string& table_name,
            const DataPage& page
        );

        bool
        table_exists(const std::string& db_name, const std::string& table_id) const;

        bool
        table_exists(
            const std::string& db_name,
            const std::string& schema_name,
            const std::string& table_name
        ) const;

        MetaTable
        load_table(const std::string& db_name, const std::string& table_id) const;

        MetaTable
        load_table(
            const std::string& db_name,
            const std::string& schema_name,
            const std::string& table_name
        ) const;

        void
        save_table(
            const std::string& db_name, const std::string& schema_name, const MetaTable& table
        );
        
        bool
        schema_exists_by_id(const std::string& db_name, const std::string& schema_id) const;
        bool
        schema_exists_by_name(const std::string& db_name, const std::string& schema_name) const;

        
        MetaSchema
        load_schema_by_id(const std::string& db_name, const std::string& schema_id) const;
        MetaSchema
        load_schema_by_name(const std::string& db_name, const std::string& schema_name) const;

        void
        save_schema(const MetaSchema& schema);

        std::vector<fs::path>
        get_tables_paths(const std::string& db_name, const std::string& schema_name);

        std::vector<fs::path>
        get_schemata_paths(const std::string& db_name);

        bool
        create_dir(const fs::path& path);

        std::pair<std::ofstream, fs::path>
        create_wal_logfile(const std::string& db_name, uint64_t first_lsn, uint64_t last_lsn);
    };
}