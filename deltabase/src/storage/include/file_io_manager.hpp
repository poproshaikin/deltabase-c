//
// Created by poproshaikin on 25.11.25.
//

#ifndef DELTABASE_FILEIOMANAGER_HPP
#define DELTABASE_FILEIOMANAGER_HPP
#include "binary_serializer.hpp"
#include "io_manager.hpp"

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

    public:
        explicit
        FileIOManager(std::unique_ptr<IBinarySerializer> serializer);

        std::vector<types::MetaTable>
        load_tables_meta() override;

        std::vector<types::MetaSchema>
        load_schemas_meta() override;

        std::vector<std::pair<types::Uuid, std::vector<types::DataPage>>>
        load_tables_data() override;
    };
}

#endif //DELTABASE_FILEIOMANAGER_HPP