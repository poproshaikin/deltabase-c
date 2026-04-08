//
// Created by poproshaikin on 25.11.25.
//

#include "include/file_io_manager.hpp"

#include "binary_serializer_factory.hpp"
#include "file_utils.hpp"
#include "path.hpp"
#include "std_storage_serializer.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <optional>

namespace storage
{
    using namespace types;
    using DbGuard = std::lock_guard<DatabaseIoLockService::Mutex>;

    FileIOManager::FileIOManager(
        const fs::path& db_path,
        const std::string& db_name,
        Config::SerializerType serializer_type
    )
        : FileIOManager(db_path, db_name, serializer_type, DatabaseIoLockService::shared())
    {
    }

    FileIOManager::FileIOManager(
        const fs::path& db_path,
        const std::string& db_name,
        Config::SerializerType serializer_type,
        std::shared_ptr<DatabaseIoLockService> io_lock_service
    )
        : db_path_(db_path), db_name_(db_name), io_lock_service_(std::move(io_lock_service))
    {
        if (!io_lock_service_)
            io_lock_service_ = DatabaseIoLockService::shared();

        BinarySerializerFactory factory;
        serializer_ = factory.make_binary_serializer(serializer_type);
        db_mutex_ = io_lock_service_->mutex_for(db_path_, db_name_);
    }

    void
    FileIOManager::init()
    {
        DbGuard guard(*db_mutex_);
        auto path = path_db(db_path_, db_name_);

        if (!fs::exists(path))
            fs::create_directories(path);
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
        for_each_in_db(
            [&](const fs::directory_entry& entry_in_db)
            {
                if (!entry_in_db.is_directory())
                    return;

                if (entry_in_db.path().filename() == PATH_WAL)
                    return;

                func(entry_in_db);
            }
        );
    }

    void
    FileIOManager::for_each_in_schema(
        const std::string& schema_name, const std::function<void(fs::directory_entry)>& func
    ) const
    {
        for_each_in_db(
            [&](const fs::directory_entry& entry)
            {
                if (entry.path().filename() != schema_name)
                    return;

                for (const auto& schema_entry : fs::directory_iterator(entry.path()))
                {
                    func(schema_entry);
                }
            }
        );
    }

    void
    FileIOManager::for_each_table(
        const std::function<void(fs::directory_entry table_dir)>& func
    ) const
    {
        for_each_in_db(
            [&](const fs::directory_entry& schema_entry)
            {
                if (!schema_entry.is_directory())
                    return;

                if (schema_entry.path().filename() == PATH_WAL)
                    return;

                for (const auto& dir_in_schema : fs::directory_iterator(schema_entry.path()))
                {
                    if (!dir_in_schema.is_directory())
                        continue;

                    func(dir_in_schema);
                }
            }
        );
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

        if (!fs::exists(path))
        {
            fs::create_directories(path);
            return;
        }

        for (const auto& entry : fs::directory_iterator(path))
        {
            if (entry.is_directory())
                continue;

            func(entry);
        }
    }

    std::vector<MetaSchema>
    FileIOManager::read_schemas_meta()
    {
        DbGuard guard(*db_mutex_);
        std::vector<MetaSchema> schemas;

        for_each_schema(
            [&](const fs::directory_entry& dir_in_db)
            {
                std::string schema_name = dir_in_db.path().filename();

                for (const auto& entry_in_schema : fs::directory_iterator(dir_in_db.path()))
                {
                    if (entry_in_schema.is_directory())
                        continue;

                    if (make_meta_filename(schema_name) != entry_in_schema.path().filename())
                        continue;

                    auto content = read_file(entry_in_schema.path());

                    auto stream = misc::ReadOnlyMemoryStream(content);
                    MetaSchema out;
                    if (!serializer_->deserialize_ms(stream, out))
                        throw std::runtime_error(
                            "FileIOManager::load_schemas: Error deserializing meta file: " +
                            path_db(db_path_, db_name_).string()
                        );

                    schemas.push_back(std::move(out));
                }
            }
        );

        return schemas;
    }

    MetaSchema
    FileIOManager::read_schema_meta(const std::string& target_schema)
    {
        DbGuard guard(*db_mutex_);
        MetaSchema schema{};
        for_each_schema(
            [&](const fs::directory_entry& schema_entry)
            {
                const auto& schema_name = schema_entry.path().filename();
                if (schema_name == target_schema)
                {
                    auto path = path_db_schema_meta(db_path_, db_name_, schema_name);
                    auto content = read_file(path);
                    auto stream = misc::ReadOnlyMemoryStream(content);

                    if (!serializer_->deserialize_ms(stream, schema))
                        throw std::runtime_error(
                            "FileIOManager::load_schemas: Error deserializing meta file: " +
                            path_db(db_path_, db_name_).string()
                        );
                }
            }
        );
        if (schema == MetaSchema{})
            throw std::runtime_error(
                "FileIOManager::load_schemas: Schema " + target_schema + " not found"
            );

        return schema;
    }

    MetaSchema
    FileIOManager::read_schema_meta(const UUID& schema_id)
    {
        DbGuard guard(*db_mutex_);
        MetaSchema schema{};

        for_each_schema(
            [&](const fs::directory_entry& schema_entry)
            {
                const auto& schema_name = schema_entry.path().filename();
                auto path = path_db_schema_meta(db_path_, db_name_, schema_name);

                auto content = read_file(path);
                auto stream = misc::ReadOnlyMemoryStream(content);

                MetaSchema deserialized;
                if (!serializer_->deserialize_ms(stream, deserialized))
                    throw std::runtime_error(
                        "FileIOManager::load_schema_meta: Error deserializing meta file: " +
                        path_db(db_path_, db_name_).string()
                    );

                if (deserialized.id == schema_id)
                    schema = std::move(deserialized);
            }
        );

        if (schema.id != schema_id)
            throw std::runtime_error(
                "FileIOManager::load_schema_meta: Failed to find schema with id " +
                schema_id.to_string()
            );

        return schema;
    }

    bool
    FileIOManager::exists_table(const std::string& table_name, const std::string& schema_name)
    {
        DbGuard guard(*db_mutex_);
        auto path = path_db_schema_table_meta(db_path_, db_name_, schema_name, table_name);
        return fs::exists(path);
    }

    std::vector<MetaTable>
    FileIOManager::read_tables_meta()
    {
        DbGuard guard(*db_mutex_);
        std::vector<MetaTable> tables;

        for_each_table(
            [&](const fs::directory_entry& table_dir)
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
                    misc::ReadOnlyMemoryStream stream(content);
                    if (!serializer_->deserialize_mt(stream, out))
                        throw std::runtime_error(
                            "FileIOManager::load_tables: Error deserializing meta file: " +
                            path_db(db_path_, db_name_).string()
                        );
                    tables.push_back(std::move(out));
                }
            }
        );

        return tables;
    }

    std::vector<std::pair<TableId, std::vector<DataPage>>>
    FileIOManager::read_tables_data()
    {
        DbGuard guard(*db_mutex_);
        std::vector<std::pair<TableId, std::vector<DataPage>>> tables;

        for_each_table(
            [&](const fs::directory_entry& table_dir)
            {
                std::string table_name = table_dir.path().filename();

                MetaTable table;
                std::vector<DataPage> data;

                for (const auto& entry_in_table : fs::directory_iterator(table_dir.path()))
                {
                    if (entry_in_table.is_directory() &&
                        entry_in_table.path().filename().string() == PATH_DATA)
                    {
                        for (const auto& entry_in_data :
                             fs::directory_iterator(entry_in_table.path()))
                        {
                            if (entry_in_data.is_directory())
                                continue;

                            auto content = read_file(entry_in_data.path());

                            DataPage page;
                            misc::ReadOnlyMemoryStream stream(content);
                            if (!serializer_->deserialize_dp(stream, page))
                                throw std::runtime_error(
                                    "FileIOManager::load_tables_data: failed to deserialize data "
                                    "page"
                                );
                            page.path = entry_in_data.path();

                            data.push_back(std::move(page));
                        }
                    }

                    if (entry_in_table.is_regular_file() &&
                        entry_in_table.path().filename().string() == make_meta_filename(table_name))
                    {
                        auto content = read_file(entry_in_table.path());

                        misc::ReadOnlyMemoryStream stream(content);
                        if (!serializer_->deserialize_mt(stream, table))
                            throw std::runtime_error(
                                "FileIOManager::load_tables_data: failed to deserialize meta table"
                            );
                    }
                }

                tables.emplace_back(std::make_pair(table.id, std::move(data)));
            }
        );

        return tables;
    }
    MetaTable
    FileIOManager::read_table_meta(const UUID& table_id)
    {
        DbGuard guard(*db_mutex_);
        std::unique_ptr<MetaTable> result;

        for_each_table(
            [this, &table_id, &result](const fs::directory_entry& table_dir)
            {
                if (result)
                    return;

                const auto table_name = table_dir.path().filename().string();
                const auto meta_path = table_dir.path() / make_meta_filename(table_name);

                if (!fs::exists(meta_path) || !fs::is_regular_file(meta_path))
                    return;

                auto content = read_file(meta_path);

                MetaTable table;
                misc::ReadOnlyMemoryStream stream(content);
                if (!serializer_->deserialize_mt(stream, table))
                    throw std::runtime_error(
                        "FileIOManager::read_table_meta: failed to deserialize meta table " +
                        meta_path.string()
                    );

                if (table.id == table_id)
                    result = std::make_unique<MetaTable>(std::move(table));
            }
        );

        if (result)
            return *result;

        throw std::runtime_error(
            "FileIOManager::read_table_meta: table with id " + table_id.to_string() + " not found"
        );
    }

    MetaTable
    FileIOManager::read_table_meta(const std::string& table_name, const std::string& schema_name)
    {
        DbGuard guard(*db_mutex_);
        auto path = path_db_schema_table_meta(db_path_, db_name_, schema_name, table_name);
        auto content = read_file(path);

        MetaTable table;
        misc::ReadOnlyMemoryStream stream(content);
        if (!serializer_->deserialize_mt(stream, table))
            throw std::runtime_error(
                "FileIOManager::load_table_meta: Error deserializing meta table " + path.string()
            );

        return table;
    }

    std::vector<DataPage>
    FileIOManager::read_table_data(const std::string& table_name, const std::string& schema_name)
    {
        DbGuard guard(*db_mutex_);
        std::vector<DataPage> pages;

        for_each_in_table_data(
            schema_name,
            table_name,
            [this, &pages](const fs::directory_entry& entry)
            {
                auto content = read_file(entry.path());

                DataPage page;
                misc::ReadOnlyMemoryStream stream(content);
                if (!serializer_->deserialize_dp(stream, page))
                    throw std::runtime_error(
                        "FileIoManager::load_table_data: failed to deserialize data page " +
                        entry.path().filename().string()
                    );

                page.path = entry.path();
                page.size = content.size();

                pages.push_back(std::move(page));
            }
        );

        return pages;
    }

    std::unique_ptr<DataPage>
    FileIOManager::read_data_page(DataPageId id)
    {
        DbGuard guard(*db_mutex_);
        const auto page_filename = id.to_string();
        std::unique_ptr<DataPage> result;

        for_each_table(
            [this, &id, &page_filename, &result](const fs::directory_entry& table_dir)
            {
                if (result)
                    return;

                auto data_dir = table_dir.path() / PATH_DATA;
                if (!fs::exists(data_dir) || !fs::is_directory(data_dir))
                    return;

                for (const auto& page_entry : fs::directory_iterator(data_dir))
                {
                    if (result)
                        return;

                    if (!page_entry.is_regular_file())
                        continue;

                    if (page_entry.path().filename().string() != page_filename)
                        continue;

                    auto content = read_file(page_entry.path());
                    DataPage page;
                    misc::ReadOnlyMemoryStream stream(content);
                    if (!serializer_->deserialize_dp(stream, page))
                        throw std::runtime_error(
                            "FileIOManager::load_data_page: failed to deserialize data page " +
                            page_entry.path().filename().string()
                        );

                    page.path = page_entry.path();
                    page.size = content.size();

                    if (page.id != id)
                        throw std::runtime_error(
                            "FileIOManager::load_data_page: page id mismatch for " +
                            page_entry.path().string()
                        );

                    result = std::make_unique<DataPage>(std::move(page));
                    return;
                }
            }
        );

        return result;
    }

    void
    FileIOManager::write_page(const DataPage& page, bool fsync)
    {
        DbGuard guard(*db_mutex_);
        auto serialized = serializer_->serialize_dp(page);
        if (fsync)
            fsync_file(page.path, serialized.to_vector());
        else
            write_file(page.path, serialized.to_vector());
    }

    uint64_t
    FileIOManager::estimate_size(const DataRow& row)
    {
        return serializer_->estimate_size(row);
    }

    void
    FileIOManager::write_mt(const MetaTable& table, const std::string& schema_name, bool fsync)
    {
        DbGuard guard(*db_mutex_);
        auto path = path_db_schema_table_meta(db_path_, db_name_, schema_name, table.name);
        auto serialized = serializer_->serialize_mt(table);
        if (fsync)
            fsync_file(path, serialized.to_vector());
        else
            write_file(path, serialized.to_vector());
    }

    void
    FileIOManager::write_mt(const MetaTable& table, bool fsync)
    {
        DbGuard guard(*db_mutex_);
        auto schema = read_schema_meta(table.schema_id);
        write_mt(table, schema.name, fsync);
    }

    void
    FileIOManager::write_ms(const MetaSchema& ms, bool fsync)
    {
        DbGuard guard(*db_mutex_);
        auto path = path_db_schema_meta(db_path_, db_name_, ms.name);
        auto serialized = serializer_->serialize_ms(ms);

        if (fsync)
            fsync_file(path, serialized.to_vector());
        else
            write_file(path, serialized.to_vector());
    }

    void
    FileIOManager::delete_mt(const MetaTable& table)
    {
        DbGuard guard(*db_mutex_);
        auto schema = read_schema_meta(table.schema_id);
        auto path = path_db_schema_table(db_path_, db_name_, schema.name, table.name);
        fs::remove_all(path);
    }

    void
    FileIOManager::delete_ms(const MetaSchema& schema)
    {
        DbGuard guard(*db_mutex_);
        auto path = path_db_schema(db_path_, db_name_, schema.name);
        fs::remove_all(path);
    }

    void
    FileIOManager::write_cfg(const Config& cfg)
    {
        DbGuard guard(*db_mutex_);
        auto path = path_db_meta(db_path_, cfg.db_name.value());
        auto serialized = serializer_->serialize_cfg(cfg);
        write_file(path, serialized.to_vector());
    }

    bool
    FileIOManager::exists_db(const std::string& name)
    {
        DbGuard guard(*db_mutex_);
        auto path = path_db(db_path_, name);
        return fs::exists(path) && fs::is_directory(path);
    }

    DataPage
    FileIOManager::create_page(const MetaTable& mt)
    {
        DbGuard guard(*db_mutex_);
        return create_page(mt, DataPageId::make());
    }

    DataPage
    FileIOManager::create_page(const MetaTable& mt, const DataPageId& page_id)
    {
        DbGuard guard(*db_mutex_);
        auto ms = read_schema_meta(mt.schema_id);
        auto data_path = path_db_schema_table_data(db_path_, db_name_, ms.name, mt.name);
        return DataPage::make(data_path, mt.id, page_id);
    }

    bool
    FileIOManager::exists_schema(const std::string& schema_name)
    {
        DbGuard guard(*db_mutex_);
        auto path = path_db_schema_meta(db_path_, db_name_, schema_name);
        return fs::exists(path) && fs::is_regular_file(path);
    }

    std::unordered_map<TableId, std::vector<DataPageId>>
    FileIOManager::map_data_pages_for_table()
    {
        DbGuard guard(*db_mutex_);
        std::unordered_map<TableId, std::vector<DataPageId>> result;

        for_each_table(
            [this, &result](const fs::directory_entry& table_dir)
            {
                const auto table_name = table_dir.path().filename().string();
                const auto meta_path = table_dir.path() / make_meta_filename(table_name);

                if (!fs::exists(meta_path) || !fs::is_regular_file(meta_path))
                    return;

                auto content = read_file(meta_path);
                MetaTable table;
                misc::ReadOnlyMemoryStream stream(content);
                if (!serializer_->deserialize_mt(stream, table))
                    throw std::runtime_error(
                        "FileIOManager::map_tables_pages: failed to deserialize meta table " +
                        meta_path.string()
                    );

                auto data_dir = table_dir.path() / PATH_DATA;
                if (!fs::exists(data_dir) || !fs::is_directory(data_dir))
                    return;

                std::vector<DataPageId> page_ids;
                for (const auto& page_entry : fs::directory_iterator(data_dir))
                {
                    if (!page_entry.is_regular_file())
                        continue;

                    page_ids.push_back(DataPageId(page_entry.path().filename().string()));
                }

                result[table.id] = std::move(page_ids);
            }
        );

        return result;
    }

    std::unordered_map<TableId, std::vector<IndexId>>
    FileIOManager::map_index_files_for_table()
    {
        DbGuard guard(*db_mutex_);
        std::unordered_map<TableId, std::vector<IndexId>> result;

        for_each_table(
            [this, &result](const fs::directory_entry& table_dir)
            {
                const auto table_name = table_dir.path().filename().string();
                const auto meta_path = table_dir.path() / make_meta_filename(table_name);

                if (!fs::exists(meta_path) || !fs::is_regular_file(meta_path))
                    return;

                auto content = read_file(meta_path);
                MetaTable table;
                misc::ReadOnlyMemoryStream stream(content);
                if (!serializer_->deserialize_mt(stream, table))
                    throw std::runtime_error(
                        "FileIOManager::map_tables_pages: failed to deserialize meta table " +
                        meta_path.string()
                    );

                auto data_dir = table_dir.path() / PATH_INDEX;
                if (!fs::exists(data_dir) || !fs::is_directory(data_dir))
                    return;

                std::vector<IndexId> page_ids;
                for (const auto& index_file_entry : fs::directory_iterator(data_dir))
                {
                    if (!index_file_entry.is_regular_file())
                        continue;

                    page_ids.push_back(IndexId(index_file_entry.path().filename().string()));
                }

                result[table.id] = std::move(page_ids);
            }
        );

        return result;
    }

    IndexFile
    FileIOManager::create_index_file(
        const std::string& schema_name, const std::string& table_name, const MetaIndex& mi
    )
    {
        DbGuard guard(*db_mutex_);
        IndexPage root;
        root.id = 1;
        root.index_id = mi.id;
        root.is_leaf = true;
        root.parent = 0;
        root.data = LeafIndexNode{ .keys = {}, .rows = {}, .next_leaf = 0};

        IndexFile file;
        file.index_id = mi.id;
        file.root_page = root.id;
        file.last_page = root.id;
        file.pages.push_back(std::move(root));

        auto serialized = serializer_->serialize_if(file);
        auto content = serialized.to_vector();

        auto path = path_db_schema_table_index(db_path_, db_name_, schema_name, table_name, mi.id.to_string());
        fs::create_directories(path.parent_path());
        write_file(path, content);

        return file;
    }

    std::unique_ptr<IndexFile>
    FileIOManager::read_index_file(const IndexId& index_id)
    {
        DbGuard guard(*db_mutex_);
        std::unique_ptr<IndexFile> result;

        for_each_table(
            [this, &index_id, &result](const fs::directory_entry& table_dir)
            {
                if (result)
                    return;

                const auto index_path = table_dir.path() / PATH_INDEX / index_id.to_string();
                if (!fs::exists(index_path) || !fs::is_regular_file(index_path))
                    return;

                auto content = read_file(index_path);

                IndexFile file;
                misc::ReadOnlyMemoryStream stream(content);
                if (!serializer_->deserialize_if(stream, file))
                    throw std::runtime_error(
                        "FileIOManager::read_index_file: failed to deserialize index file " +
                        index_path.string()
                    );

                if (file.index_id != index_id)
                    throw std::runtime_error(
                        "FileIOManager::read_index_file: index id mismatch for " +
                        index_path.string()
                    );

                result = std::make_unique<IndexFile>(std::move(file));
            }
        );

        return result;
    }

    void
    FileIOManager::write_index_file(const IndexFile& index_file, bool fsync)
    {
        DbGuard guard(*db_mutex_);
        std::optional<fs::path> index_path;

        for_each_table(
            [&index_file, &index_path](const fs::directory_entry& table_dir)
            {
                if (index_path)
                    return;

                const auto candidate = table_dir.path() / PATH_INDEX / index_file.index_id.to_string();
                if (fs::exists(candidate) && fs::is_regular_file(candidate))
                    index_path = candidate;
            }
        );

        if (!index_path)
            throw std::runtime_error(
                "FileIOManager::write_index_file: index file with id " +
                index_file.index_id.to_string() + " not found"
            );

        auto serialized = serializer_->serialize_if(index_file);
        write_file(*index_path, serialized.to_vector());
    }
} // namespace storage