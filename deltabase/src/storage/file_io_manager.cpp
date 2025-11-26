//
// Created by poproshaikin on 25.11.25.
//

#include "include/file_io_manager.hpp"
#include "path.hpp"

#include <chrono>
#include <fstream>

namespace storage
{
    using namespace types;

    FileIOManager::FileIOManager(std::unique_ptr<IBinarySerializer> serializer) : serializer_(
        std::move(serializer))
    {
    }

    Bytes
    FileIOManager::read_file(const fs::path& path) const
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file)
        {
            throw std::runtime_error("Cannot open file: " + std::string(path));
        }

        std::streamsize size = file.tellg(); // file size
        file.seekg(0, std::ios::beg); // return to start

        std::vector<uint8_t> buffer(size);

        if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
        {
            throw std::runtime_error("Error reading file: " + std::string(path));
        }

        return buffer;
    }

    void
    FileIOManager::for_each_in_db(const std::function<void(fs::directory_entry)>& func) const
    {
        auto path = path_db(db_path_, db_name_);

        for (const auto& entry : fs::directory_iterator(path))
        {
            func(entry);
        }
    }

    void
    FileIOManager::for_each_schema(const std::function<void(fs::directory_entry)>& func) const
    {
        for_each_in_db([&](const fs::directory_entry& entry_in_db)
        {
            if (!entry_in_db.is_directory())
                return;

            func(entry_in_db);
        });
    }

    void
    FileIOManager::for_each_in_schema(
        const std::string& schema_name,
        const std::function<void(fs::directory_entry)>& func
    ) const
    {
        for_each_in_db([&](const fs::directory_entry& entry)
        {
            if (entry.path().filename() != schema_name)
                return;

            for (const auto& schema_entry : fs::directory_iterator(entry.path()))
            {
                func(schema_entry);
            }
        });
    }

    void
    FileIOManager::for_each_table(const std::function<void(fs::directory_entry)>& func) const
    {
        for_each_in_db([&](const fs::directory_entry& schema_entry)
        {
            if (!schema_entry.is_directory())
                return;

            for (const auto& dir_in_schema : fs::directory_iterator(schema_entry.path()))
            {
                if (!dir_in_schema.is_directory())
                    continue;

                func(dir_in_schema);
            }
        });
    }

    void
    FileIOManager::for_each_in_table(
        const std::string& schema_name,
        const std::string& table_name,
        const std::function<void(fs::directory_entry)>& func
    ) const
    {
        for_each_in_schema(
            schema_name,
            [&](const fs::directory_entry& schema_entry)
            {
                if (!schema_entry.is_directory())
                    return;

                if (schema_entry.path().filename() != table_name)
                    return;

                for (const auto& entry_in_table : fs::directory_iterator(schema_entry.path()))
                {
                    func(entry_in_table);
                }
            }
        );
    }

    std::vector<MetaSchema>
    FileIOManager::load_schemas_meta()
    {
        std::vector<MetaSchema> schemas;

        for_each_schema([&](const fs::directory_entry& dir_in_db)
        {
            std::string schema_name = dir_in_db.path().filename();

            for (const auto& entry_in_schema : fs::directory_iterator(dir_in_db.path()))
            {
                if (entry_in_schema.is_directory())
                    continue;

                if (make_meta_filename(schema_name) != entry_in_schema.path().filename())
                    continue;

                auto content = read_file(entry_in_schema.path());

                MetaSchema out;
                if (!serializer_->deserialize_ms(content, out))
                    throw std::runtime_error(
                        "FileIOManager::load_schemas: Error deserializing meta file: " +
                        path_db(db_path_, db_name_).string()
                    );

                schemas.push_back(std::move(out));
            }
        });

        return schemas;
    }

    std::vector<MetaTable>
    FileIOManager::load_tables_meta()
    {
        std::vector<MetaTable> tables;

        for_each_table([&](const fs::directory_entry& table_dir)
        {
            std::string table_name = table_dir.path().filename();

            for (const auto& entry_in_table : fs::directory_iterator(table_dir.path()))
            {
                if (entry_in_table.is_directory())
                    continue;

                if (make_meta_filename(table_name) != entry_in_table.path().filename())
                    continue;

                auto content = read_file(entry_in_table.path());

                MetaTable out;
                if (!serializer_->deserialize_mt(content, out))
                    throw std::runtime_error(
                        "FileIOManager::load_tables: Error deserializing meta file: " +
                        path_db(db_path_, db_name_).string()
                    );
                tables.push_back(std::move(out));
            }
        });

        return tables;
    }

    std::vector<std::pair<Uuid, std::vector<DataPage> > >
    FileIOManager::load_tables_data()
    {
        std::vector<std::pair<Uuid, std::vector<DataPage> > > tables;

        for_each_table([&](const fs::directory_entry& table_dir)
        {
            std::string table_name = table_dir.path().filename();

            MetaTable table;
            std::vector<DataPage> data;

            for (const auto& entry_in_table : fs::directory_iterator(table_dir.path()))
            {
                if (
                    entry_in_table.is_directory() &&
                    entry_in_table.path().filename().string() == PATH_DATA
                )
                {
                    for (const auto& entry_in_data : fs::directory_iterator(entry_in_table.path()))
                    {
                        if (entry_in_data.is_directory())
                            continue;

                        DataPage page;
                        if (!serializer_.deserialize_dp())
                            throw std::runtime_error("FileIOManager::load_tables_data: failed to deserialize data page");
                    }
                }
            }
        });
    }
}