//
// Created by poproshaikin on 25.11.25.
//

#ifndef DELTABASE_FILEIOMANAGER_HPP
#define DELTABASE_FILEIOMANAGER_HPP
#include "binary_serializer.hpp"
#include "io_manager.hpp"
#include "../../types/include/config.hpp"

#include <filesystem>
#include <functional>

namespace storage
{
    namespace fs = std::filesystem;

    class FileIOManager final : public IIOManager
    {
        std::string db_name_;
        fs::path db_path_;
        std::unique_ptr<IBinarySerializer> serializer_;

        void
        for_each_in_db(const std::function<void(fs::directory_entry)>& func) const;

        void
        for_each_schema(const std::function<void(fs::directory_entry)>& func) const;

        void
        for_each_in_schema(
            const std::string& schema_name,
            const std::function<void(fs::directory_entry)>& func
        ) const;

        void
        for_each_table(const std::function<void(fs::directory_entry)>& func) const;

        void
        for_each_in_table(
            const std::string& schema_name,
            const std::string& table_name,
            const std::function<void(fs::directory_entry)>& func
        ) const;

        void
        for_each_in_table_data(
            const std::string& schema_name,
            const std::string& table_name,
            const std::function<void(fs::directory_entry)>& func
        ) const;

    public:
        explicit
        FileIOManager(const fs::path& db_path, const std::string& db_name, types::Config::SerializerType serializer_type);

        void
        init() override;

        std::vector<types::MetaTable>
        read_tables_meta() override;

        std::vector<types::MetaSchema>
        read_schemas_meta() override;

        types::MetaSchema
        read_schema_meta(const std::string& target_schema) override;

        types::MetaSchema
        read_schema_meta(const types::Uuid& schema_id) override;

        bool
        exists_table(const std::string& table_name, const std::string& schema_name) override;

        std::vector<std::pair<types::Uuid, std::vector<types::DataPage> > >
        read_tables_data() override;

        types::MetaTable
        read_table_meta(const types::Uuid& table_id) override;

        types::MetaTable
        read_table_meta(const std::string& table_name, const std::string& schema_name) override;

        std::vector<types::DataPage>
        read_table_data(const std::string& table_name, const std::string& schema_name) override;

        std::unique_ptr<types::DataPage>
        read_data_page(types::PageId id) override;

        void
        write_page(const types::DataPage& page, bool fsync) override;

        uint64_t
        estimate_size(const types::DataRow& row) override;

        void
        write_mt(const types::MetaTable& table, const std::string& schema_name, bool fsync) override;

        void
        write_mt(const types::MetaTable& table, bool fsync) override;

        void
        write_cfg(const types::Config& cfg) override;

        bool
        exists_db(const std::string& name) override;

        void
        write_ms(const types::MetaSchema& ms, bool fsync) override;

        void
        delete_mt(const types::MetaTable& table) override;

        void
        delete_ms(const types::MetaSchema& schema) override;

        types::DataPage
        create_page(const types::MetaTable& mt) override;

        types::DataPage
        create_page(const types::MetaTable& mt, const types::PageId& page_id) override;

        bool
        exists_schema(const std::string& schema_name) override;

        std::unordered_map<types::TableId, std::vector<types::PageId>>
        map_tables_pages() override;
    };
}

#endif //DELTABASE_FILEIOMANAGER_HPP