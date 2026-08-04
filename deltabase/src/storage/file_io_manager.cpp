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
        : db_path_(db_path),
          db_name_(db_name),
          io_lock_service_(std::move(io_lock_service)),
          paths_(db_path, db_name)
    {
        if (!io_lock_service_)
            io_lock_service_ = DatabaseIoLockService::shared();

        StorageSerializerFactory factory;
        serializer_ = factory.make_binary_serializer(serializer_type);
        db_mutex_ = io_lock_service_->mutex_for(db_path_, db_name_);
    }

    void
    FileIOManager::init()
    {
        DbGuard guard(*db_mutex_);
        auto path = paths_.db();
        if (!fs::exists(path))
            fs::create_directories(path);
    }

    void
    FileIOManager::init_wal()
    {
        DbGuard guard(*db_mutex_);
        auto wal_dir = paths_.wal();
        if (!fs::exists(wal_dir))
            fs::create_directories(wal_dir);
    }

    // --- iteration helpers ---------------------------------------------------

    void
    FileIOManager::for_each_in_db(const std::function<void(fs::directory_entry)>& func) const
    {
        for (const auto& entry : fs::directory_iterator(paths_.db()))
            func(entry);
    }

    void
    FileIOManager::for_each_schema(const std::function<void(fs::directory_entry)>& func) const
    {
        for_each_in_db(
            [&](const fs::directory_entry& entry)
            {
                if (!entry.is_directory()) return;
                if (entry.path().filename() == PATH_WAL) return;
                func(entry);
            }
        );
    }

    // Iterates every table directory across all schemas.
    // Callback receives entries at: {db}/{schema}/tables/{table}
    void
    FileIOManager::for_each_table(const std::function<void(fs::directory_entry)>& func) const
    {
        for_each_schema(
            [&](const fs::directory_entry& schema_entry)
            {
                const auto tables_path = paths_.tables_dir(
                    schema_entry.path().filename().string()
                );
                if (!fs::exists(tables_path))
                    return;

                for (const auto& table_entry : fs::directory_iterator(tables_path))
                {
                    if (table_entry.is_directory())
                        func(table_entry);
                }
            }
        );
    }

    // --- schema reads --------------------------------------------------------

    std::vector<MetaSchema>
    FileIOManager::read_schemas_meta()
    {
        DbGuard guard(*db_mutex_);
        std::vector<MetaSchema> schemas;

        for_each_schema(
            [&](const fs::directory_entry& schema_entry)
            {
                const auto schema_name = schema_entry.path().filename().string();
                const auto meta_path = paths_.schema_meta(schema_name);

                if (!fs::exists(meta_path))
                    return;

                auto content = read_file(meta_path);
                auto stream = misc::ReadOnlyMemoryStream(content);
                MetaSchema ms;
                if (!serializer_->deserialize_ms(stream, ms))
                    throw std::runtime_error(
                        "FileIOManager::read_schemas_meta: failed to deserialize " +
                        meta_path.string()
                    );
                schemas.push_back(std::move(ms));
            }
        );

        return schemas;
    }

    MetaSchema
    FileIOManager::read_schema_meta(const std::string& target_schema)
    {
        DbGuard guard(*db_mutex_);
        const auto path = paths_.schema_meta(target_schema);
        auto content = read_file(path);
        auto stream = misc::ReadOnlyMemoryStream(content);
        MetaSchema schema;
        if (!serializer_->deserialize_ms(stream, schema))
            throw std::runtime_error(
                "FileIOManager::read_schema_meta: failed to deserialize " + path.string()
            );
        return schema;
    }

    MetaSchema
    FileIOManager::read_schema_meta(const UUID& schema_id)
    {
        DbGuard guard(*db_mutex_);
        MetaSchema result{};

        for_each_schema(
            [&](const fs::directory_entry& schema_entry)
            {
                if (result.id == schema_id)
                    return;

                const auto schema_name = schema_entry.path().filename().string();
                const auto path = paths_.schema_meta(schema_name);
                auto content = read_file(path);
                auto stream = misc::ReadOnlyMemoryStream(content);

                MetaSchema ms;
                if (!serializer_->deserialize_ms(stream, ms))
                    throw std::runtime_error(
                        "FileIOManager::read_schema_meta: failed to deserialize " + path.string()
                    );

                if (ms.id == schema_id)
                    result = std::move(ms);
            }
        );

        if (result.id != schema_id)
            throw std::runtime_error(
                "FileIOManager::read_schema_meta: schema with id " + schema_id.to_string() +
                " not found"
            );

        return result;
    }

    bool
    FileIOManager::exists_schema(const std::string& schema_name)
    {
        DbGuard guard(*db_mutex_);
        const auto path = paths_.schema_meta(schema_name);
        return fs::exists(path) && fs::is_regular_file(path);
    }

    // --- table reads ---------------------------------------------------------

    bool
    FileIOManager::exists_table(const std::string& table_name, const std::string& schema_name)
    {
        DbGuard guard(*db_mutex_);
        return fs::exists(paths_.table_meta(schema_name, table_name));
    }

    std::vector<MetaTable>
    FileIOManager::read_tables_meta()
    {
        DbGuard guard(*db_mutex_);
        std::vector<MetaTable> tables;

        for_each_schema(
            [&](const fs::directory_entry& schema_entry)
            {
                const auto schema_name = schema_entry.path().filename().string();
                const auto tables_path = paths_.tables_dir(schema_name);
                if (!fs::exists(tables_path))
                    return;

                for (const auto& table_entry : fs::directory_iterator(tables_path))
                {
                    if (!table_entry.is_directory())
                        continue;

                    const auto table_name = table_entry.path().filename().string();
                    const auto meta_path = paths_.table_meta(schema_name, table_name);
                    if (!fs::exists(meta_path))
                        continue;

                    auto content = read_file(meta_path);
                    MetaTable mt;
                    misc::ReadOnlyMemoryStream stream(content);
                    if (!serializer_->deserialize_mt(stream, mt))
                        throw std::runtime_error(
                            "FileIOManager::read_tables_meta: failed to deserialize " +
                            meta_path.string()
                        );
                    tables.push_back(std::move(mt));
                }
            }
        );

        return tables;
    }

    MetaTable
    FileIOManager::read_table_meta(const std::string& table_name, const std::string& schema_name)
    {
        DbGuard guard(*db_mutex_);
        const auto path = paths_.table_meta(schema_name, table_name);
        auto content = read_file(path);
        MetaTable table;
        misc::ReadOnlyMemoryStream stream(content);
        if (!serializer_->deserialize_mt(stream, table))
            throw std::runtime_error(
                "FileIOManager::read_table_meta: failed to deserialize " + path.string()
            );
        return table;
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
                MetaTable mt;
                misc::ReadOnlyMemoryStream stream(content);
                if (!serializer_->deserialize_mt(stream, mt))
                    throw std::runtime_error(
                        "FileIOManager::read_table_meta: failed to deserialize " +
                        meta_path.string()
                    );

                if (mt.id == table_id)
                    result = std::make_unique<MetaTable>(std::move(mt));
            }
        );

        if (result)
            return *result;

        throw std::runtime_error(
            "FileIOManager::read_table_meta: table with id " + table_id.to_string() + " not found"
        );
    }

    // --- data reads ----------------------------------------------------------

    std::vector<std::pair<TableId, std::vector<DataPage>>>
    FileIOManager::read_tables_data()
    {
        DbGuard guard(*db_mutex_);
        std::vector<std::pair<TableId, std::vector<DataPage>>> result;

        for_each_schema(
            [&](const fs::directory_entry& schema_entry)
            {
                const auto schema_name = schema_entry.path().filename().string();
                const auto tables_path = paths_.tables_dir(schema_name);
                if (!fs::exists(tables_path))
                    return;

                for (const auto& table_entry : fs::directory_iterator(tables_path))
                {
                    if (!table_entry.is_directory())
                        continue;

                    const auto table_name = table_entry.path().filename().string();
                    const auto meta_path = paths_.table_meta(schema_name, table_name);
                    if (!fs::exists(meta_path))
                        continue;

                    MetaTable mt;
                    {
                        auto content = read_file(meta_path);
                        misc::ReadOnlyMemoryStream stream(content);
                        if (!serializer_->deserialize_mt(stream, mt))
                            throw std::runtime_error(
                                "FileIOManager::read_tables_data: failed to deserialize " +
                                meta_path.string()
                            );
                    }

                    std::vector<DataPage> pages;
                    const auto data_dir = paths_.table_data_dir(schema_name, table_name);
                    if (fs::exists(data_dir) && fs::is_directory(data_dir))
                    {
                        for (const auto& page_entry : fs::directory_iterator(data_dir))
                        {
                            if (!page_entry.is_regular_file())
                                continue;

                            auto content = read_file(page_entry.path());
                            DataPage page;
                            misc::ReadOnlyMemoryStream stream(content);
                            if (!serializer_->deserialize_dp(stream, page))
                                throw std::runtime_error(
                                    "FileIOManager::read_tables_data: failed to deserialize data page"
                                );
                            page.path = page_entry.path();
                            pages.push_back(std::move(page));
                        }
                    }

                    result.emplace_back(mt.id, std::move(pages));
                }
            }
        );

        return result;
    }

    std::vector<DataPage>
    FileIOManager::read_table_data(const std::string& table_name, const std::string& schema_name)
    {
        DbGuard guard(*db_mutex_);
        std::vector<DataPage> pages;

        const auto data_path = paths_.table_data_dir(schema_name, table_name);
        if (!fs::exists(data_path))
        {
            fs::create_directories(data_path);
            return pages;
        }

        for (const auto& entry : fs::directory_iterator(data_path))
        {
            if (entry.is_directory())
                continue;

            auto content = read_file(entry.path());
            DataPage page;
            misc::ReadOnlyMemoryStream stream(content);
            if (!serializer_->deserialize_dp(stream, page))
                throw std::runtime_error(
                    "FileIOManager::read_table_data: failed to deserialize data page " +
                    entry.path().filename().string()
                );

            page.path = entry.path();
            page.size = content.size();
            pages.push_back(std::move(page));
        }

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

                const auto data_dir = table_dir.path() / PATH_DATA;
                if (!fs::exists(data_dir) || !fs::is_directory(data_dir))
                    return;

                const auto page_path = data_dir / page_filename;
                if (!fs::exists(page_path) || !fs::is_regular_file(page_path))
                    return;

                auto content = read_file(page_path);
                DataPage page;
                misc::ReadOnlyMemoryStream stream(content);
                if (!serializer_->deserialize_dp(stream, page))
                    throw std::runtime_error(
                        "FileIOManager::read_data_page: failed to deserialize data page " +
                        page_filename
                    );

                page.path = page_path;
                page.size = content.size();

                if (page.id != id)
                    throw std::runtime_error(
                        "FileIOManager::read_data_page: page id mismatch for " +
                        page_path.string()
                    );

                result = std::make_unique<DataPage>(std::move(page));
            }
        );

        return result;
    }

    // --- writes --------------------------------------------------------------

    void
    FileIOManager::write(const DataPage& page, bool fsync)
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
        const auto path = paths_.table_meta(schema_name, table.name);
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
        const auto path = paths_.schema_meta(ms.name);
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
        fs::remove_all(paths_.table(schema.name, table.name));
    }

    void
    FileIOManager::delete_ms(const MetaSchema& schema)
    {
        DbGuard guard(*db_mutex_);
        fs::remove_all(paths_.schema(schema.name));
    }

    void
    FileIOManager::write_cfg(const Config& cfg)
    {
        DbGuard guard(*db_mutex_);
        auto serialized = serializer_->serialize_cfg(cfg);
        write_file(paths_.db_meta(), serialized.to_vector());
    }

    bool
    FileIOManager::exists_db(const std::string& name)
    {
        DbGuard guard(*db_mutex_);
        const auto path = db_path_ / name;
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
        const auto data_path = paths_.table_data_dir(ms.name, mt.name);
        return DataPage::make(data_path, mt.id, page_id);
    }

    // --- index maps ----------------------------------------------------------

    std::unordered_map<TableId, std::vector<DataPageId>>
    FileIOManager::map_data_pages_for_table()
    {
        DbGuard guard(*db_mutex_);
        std::unordered_map<TableId, std::vector<DataPageId>> result;

        for_each_schema(
            [&](const fs::directory_entry& schema_entry)
            {
                const auto schema_name = schema_entry.path().filename().string();
                const auto tables_path = paths_.tables_dir(schema_name);
                if (!fs::exists(tables_path))
                    return;

                for (const auto& table_entry : fs::directory_iterator(tables_path))
                {
                    if (!table_entry.is_directory())
                        continue;

                    const auto table_name = table_entry.path().filename().string();
                    const auto meta_path = paths_.table_meta(schema_name, table_name);
                    if (!fs::exists(meta_path))
                        continue;

                    MetaTable mt;
                    {
                        auto content = read_file(meta_path);
                        misc::ReadOnlyMemoryStream stream(content);
                        if (!serializer_->deserialize_mt(stream, mt))
                            throw std::runtime_error(
                                "FileIOManager::map_data_pages_for_table: failed to deserialize " +
                                meta_path.string()
                            );
                    }

                    const auto data_dir = paths_.table_data_dir(schema_name, table_name);
                    if (!fs::exists(data_dir) || !fs::is_directory(data_dir))
                        continue;

                    std::vector<DataPageId> page_ids;
                    for (const auto& page_entry : fs::directory_iterator(data_dir))
                    {
                        if (!page_entry.is_regular_file())
                            continue;
                        page_ids.push_back(DataPageId(page_entry.path().filename().string()));
                    }

                    result[mt.id] = std::move(page_ids);
                }
            }
        );

        return result;
    }

    std::unordered_map<TableId, std::vector<IndexId>>
    FileIOManager::map_index_files_for_table()
    {
        DbGuard guard(*db_mutex_);
        std::unordered_map<TableId, std::vector<IndexId>> result;

        for_each_schema(
            [&](const fs::directory_entry& schema_entry)
            {
                const auto schema_name = schema_entry.path().filename().string();
                const auto tables_path = paths_.tables_dir(schema_name);
                if (!fs::exists(tables_path))
                    return;

                for (const auto& table_entry : fs::directory_iterator(tables_path))
                {
                    if (!table_entry.is_directory())
                        continue;

                    const auto table_name = table_entry.path().filename().string();
                    const auto meta_path = paths_.table_meta(schema_name, table_name);
                    if (!fs::exists(meta_path))
                        continue;

                    MetaTable mt;
                    {
                        auto content = read_file(meta_path);
                        misc::ReadOnlyMemoryStream stream(content);
                        if (!serializer_->deserialize_mt(stream, mt))
                            throw std::runtime_error(
                                "FileIOManager::map_index_files_for_table: failed to deserialize " +
                                meta_path.string()
                            );
                    }

                    const auto index_dir = paths_.table_index_dir(schema_name, table_name);
                    if (!fs::exists(index_dir) || !fs::is_directory(index_dir))
                        continue;

                    std::vector<IndexId> index_ids;
                    for (const auto& index_entry : fs::directory_iterator(index_dir))
                    {
                        if (!index_entry.is_regular_file())
                            continue;
                        index_ids.push_back(IndexId(index_entry.path().filename().string()));
                    }

                    result[mt.id] = std::move(index_ids);
                }
            }
        );

        return result;
    }

    // --- index files ---------------------------------------------------------

    types::IndexFile
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

        const auto path = paths_.table_index(schema_name, table_name, mi.id.to_string());
        fs::create_directories(path.parent_path());
        auto serialized = serializer_->serialize_if(file);
        write_file(path, serialized.to_vector());

        return file;
    }

    std::unique_ptr<types::IndexFile>
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
                        "FileIOManager::read_index_file: failed to deserialize " +
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
    FileIOManager::write(const IndexFile& index_file, bool fsync)
    {
        DbGuard guard(*db_mutex_);
        std::optional<fs::path> index_path;

        for_each_table(
            [&index_file, &index_path](const fs::directory_entry& table_dir)
            {
                if (index_path)
                    return;

                const auto candidate =
                    table_dir.path() / PATH_INDEX / index_file.index_id.to_string();
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

    // --- sequences -----------------------------------------------------------

    MetaSequence
    FileIOManager::read_seq(const std::string& name, const std::string& schema_name)
    {
        DbGuard guard(*db_mutex_);
        const auto path = paths_.sequence(schema_name, name);
        auto content = read_file(path);
        MetaSequence sequence;
        misc::ReadOnlyMemoryStream stream(content);
        if (!serializer_->deserialize_seq(stream, sequence))
            throw std::runtime_error(
                "FileIOManager::read_seq: failed to deserialize sequence " + name
            );
        return sequence;
    }

    void
    FileIOManager::write_seq(const MetaSequence& sequence, bool fsync)
    {
        DbGuard guard(*db_mutex_);
        const auto path = paths_.sequence(sequence.schema_name, sequence.name);
        fs::create_directories(path.parent_path());
        auto serialized = serializer_->serialize_seq(sequence);
        write_file(path, serialized.to_vector());
    }

    void
    FileIOManager::delete_seq(const MetaSequence& sequence)
    {
        DbGuard guard(*db_mutex_);
        const auto path = paths_.sequence(sequence.schema_name, sequence.name);
        if (fs::exists(path))
            fs::remove(path);
    }

    std::vector<MetaSequence>
    FileIOManager::read_sequences()
    {
        DbGuard guard(*db_mutex_);
        std::vector<MetaSequence> sequences;

        for_each_schema(
            [&](const fs::directory_entry& schema_entry)
            {
                const auto schema_name = schema_entry.path().filename().string();
                const auto seq_dir = paths_.sequences_dir(schema_name);
                if (!fs::exists(seq_dir) || !fs::is_directory(seq_dir))
                    return;

                for (const auto& seq_entry : fs::directory_iterator(seq_dir))
                {
                    if (!seq_entry.is_regular_file())
                        continue;

                    auto content = read_file(seq_entry.path());
                    MetaSequence seq;
                    misc::ReadOnlyMemoryStream stream(content);
                    if (!serializer_->deserialize_seq(stream, seq))
                        throw std::runtime_error(
                            "FileIOManager::read_sequences: failed to deserialize " +
                            seq_entry.path().filename().string()
                        );
                    seq.schema_name = schema_name;
                    sequences.push_back(std::move(seq));
                }
            }
        );

        return sequences;
    }
} // namespace storage
