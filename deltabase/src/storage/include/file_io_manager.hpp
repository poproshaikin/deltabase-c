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

        types::Bytes
        read_file(const fs::path& path) const;

        void
        write_file(const fs::path& path, const types::Bytes& content) const;

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
        FileIOManager(const fs::path& db_path, types::Config::SerializerType serializer_type);

        void
        init() override;

        constexpr uint64_t
        max_dp_size() override;

        std::vector<types::MetaTable>
        load_tables_meta() override;

        std::vector<types::MetaSchema>
        load_schemas_meta() override;

        types::MetaSchema
        load_schema_meta(const std::string& target_schema) override;

        bool
        exists_table(const std::string& string, const std::string& schema_name) override;

        std::vector<std::pair<types::Uuid, std::vector<types::DataPage> > >
        load_tables_data() override;

        types::MetaTable
        load_table_meta(const std::string& table_name, const std::string& schema_name) override;

        std::vector<types::DataPage>
        load_table_data(const std::string& table_name, const std::string& schema_name) override;

        void
        write_page(const types::DataPage& page) override;

        uint64_t
        estimate_size(const types::DataRow& row) override;

        void
        write_mt(const types::MetaTable& table, const std::string& schema_name) override;

        void
        write_cfg(const types::Config& db) override;

        bool
        exists_db(const std::string& name) override;
    };
}

#endif //DELTABASE_FILEIOMANAGER_HPP