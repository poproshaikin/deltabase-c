//
// Created by poproshaikin on 25.11.25.
//

#include "include/file_io_manager.hpp"
#include "path.hpp"
#include "std_binary_serializer.hpp"

#include <chrono>
#include <fstream>

namespace storage
{
    using namespace types;

    FileIOManager::FileIOManager(
        const fs::path& db_path,
        Config::SerializerType serializer_type
    ) : db_path_(db_path)
    {
        switch (serializer_type)
        {
        case Config::SerializerType::Std:
            serializer_ = std::make_unique<StdBinarySerializer>(StdBinarySerializer());
            break;
        default:
            throw std::runtime_error(
                "FileIOManager::FileIOManager: unknown serializer type " + std::to_string(
                    static_cast<int>(serializer_type)));
        }
    }

    void
    FileIOManager::init()
    {
        auto path = path_db(db_path_, db_name_);

        if (!fs::exists(path))
            fs::create_directories(path);
    }

    Bytes
    FileIOManager::read_file(const fs::path& path) const
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file)
            throw std::runtime_error("Cannot open file: " + std::string(path));

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
    FileIOManager::write_file(const fs::path& path, const types::Bytes& content) const
    {
        std::ofstream file(path, std::ios::binary);
        file.write(reinterpret_cast<const char*>(content.data()), content.size());
        file.close();
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

    void
    FileIOManager::for_each_in_table_data(
        const std::string& schema_name,
        const std::string& table_name,
        const std::function<void(fs::directory_entry)>& func
    ) const
    {
        auto path = path_db_schema_table_data(db_path_, db_name_, schema_name, table_name);

        for (const auto& entry : fs::directory_iterator(path))
        {
            if (entry.is_directory())
                continue;

            func(entry);
        }
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
                if (entry_in_table.is_directory() &&
                    entry_in_table.path().filename().string() == PATH_DATA)
                {
                    for (const auto& entry_in_data : fs::directory_iterator(entry_in_table.path()))
                    {
                        if (entry_in_data.is_directory())
                            continue;

                        auto content = read_file(entry_in_data.path());

                        DataPage page;
                        if (!serializer_->deserialize_dp(content, page))
                            throw std::runtime_error(
                                "FileIOManager::load_tables_data: failed to deserialize data page");
                        page.path = entry_in_data.path();

                        data.push_back(std::move(page));
                    }
                }

                if (entry_in_table.is_regular_file() &&
                    entry_in_table.path().filename().string() == make_meta_filename(table_name))
                {
                    auto content = read_file(entry_in_table.path());

                    if (!serializer_->deserialize_mt(content, table))
                        throw std::runtime_error(
                            "FileIOManager::load_tables_data: failed to deserialize meta table");
                }
            }

            tables.emplace_back(std::make_pair(table.id, std::move(data)));
        });

        return tables;
    }

    MetaTable
    FileIOManager::load_table_meta(const std::string& table_name, const std::string& schema_name)
    {
        auto path = path_db_schema_table_meta(db_path_, db_name_, schema_name, table_name);
        auto content = read_file(path);

        MetaTable table;
        if (!serializer_->deserialize_mt(content, table))
            throw std::runtime_error(
                "FileIOManager::load_table_meta: Error deserializing meta table " + path.string());

        return table;
    }

    std::vector<DataPage>
    FileIOManager::load_table_data(const std::string& table_name, const std::string& schema_name)
    {
        std::vector<DataPage> pages;

        for_each_in_table_data(
            schema_name,
            table_name,
            [this, &pages](const fs::directory_entry& entry)
            {
                auto content = read_file(entry.path());

                DataPage page;
                if (!serializer_->deserialize_dp(content, page))
                    throw std::runtime_error(
                        "FileIoManager::load_table_data: failed to deserialize data page + "
                        + entry.path().filename().string());
                page.path = entry.path();

                pages.push_back(std::move(page));
            }
        );

        return pages;
    }

    void
    FileIOManager::write_page(const DataPage& page)
    {
        auto serialized = serializer_->serialize_dp(page);
        write_file(page.path, serialized);
    }

    constexpr uint64_t
    FileIOManager::max_dp_size()
    {
        return 16 * 1024; // 16 Kb
    }

    uint64_t
    FileIOManager::estimate_size(const DataRow& row)
    {

    }

    void
    FileIOManager::write_mt(const MetaTable& table, const std::string& schema_name)
    {
        auto path = path_db_schema_table_meta(db_path_, db_name_, schema_name, table.name);
        auto serialized = serializer_->serialize_mt(table);
        write_file(path, serialized);
    }

    void
    FileIOManager::write_cfg(const Config& db)
    {
        auto path = path_db_meta(db_path_, db.name);
        auto serialized = serializer_->serialize_cfg(db);
        write_file(path, serialized);
    }
}