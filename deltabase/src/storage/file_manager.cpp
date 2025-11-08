#include "include/file_manager.hpp"
#include "include/objects/meta_object.hpp"
#include "include/paths.hpp"
#include "include/wal/wal_object.hpp"
#include "../misc/include/utils.hpp"

#include <bits/fs_fwd.h>
#include <bits/fs_path.h>
#include <complex.h>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace storage
{

    FileManager::FileManager(const fs::path& data_dir) : data_dir_(data_dir)
    {
        if (!fs::exists(data_dir_))
            fs::create_directories(data_dir_);
    }

    bytes_v
    FileManager::read_file(const fs::path& path) const
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
            throw std::runtime_error("Cannot open file: " + path.string());

        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);

        bytes_v content(size);
        file.read(reinterpret_cast<char*>(content.data()), size);

        if (!file)
            throw std::runtime_error("Error reading file: " + path.string());

        return content;
    }

    void
    FileManager::write_file(const fs::path& path, const bytes_v& content)
    {
        if (path.has_parent_path())
            fs::create_directories(path.parent_path());

        std::ofstream file(path, std::ios::binary);
        if (!file)
            throw std::runtime_error("Cannot create file: " + path.string());

        file.write(reinterpret_cast<const char*>(content.data()), content.size());
        if (!file)
            throw std::runtime_error("Error writing file: " + path.string());
    }

    std::optional<fs::path>
    FileManager::find_page_path(const std::string& db_name, const std::string& page_id) const
    {
        auto db_path = path_db(data_dir_, db_name);

        for (const auto& schema_entry : fs::directory_iterator(db_path))
        {
            if (!schema_entry.is_directory())
                continue;

            for (const auto& table_entry : fs::directory_iterator(schema_entry.path()))
            {
                if (!table_entry.is_directory())
                    continue;

                auto table_data_path = path_db_schema_table_data(
                    data_dir_,
                    db_name,
                    schema_entry.path().filename(),
                    table_entry.path().filename()
                );

                for (const auto& page_entry : fs::directory_iterator(table_data_path))
                {
                    if (!page_entry.is_regular_file())
                        continue;

                    if (page_entry.path().filename() == page_id)
                        return page_entry.path();
                }
            }
        }

        return std::nullopt;
    }

    bool
    FileManager::exists_page(const std::string& db_name, const std::string& page_id) const
    {
        return find_page_path(db_name, page_id) != std::nullopt;
    }

    DataPage
    FileManager::load_page(const std::string& db_name, const std::string& page_id) const
    {
        auto path = find_page_path(db_name, page_id);

        if (path == std::nullopt)
            throw std::runtime_error(
                "FileManager::load_page: page " + page_id + "doesnt exists in db " + db_name);

        bytes_v bytes = read_file(path.value());
        DataPage page;
        if (!DataPage::try_deserialize(bytes, page))
            throw std::runtime_error(
                "FileManager::load_page: failed to deserialize page " + page_id +
                ": data are corrupted");

        return page;
    }

    bool
    FileManager::exists_page(
        const std::string& db_name,
        const std::string& schema_name,
        const std::string& table_name,
        const std::string& page_id
    ) const
    {
        auto path = path_db_schema_table_page(data_dir_, db_name, schema_name, table_name, page_id);
        return fs::exists(path);
    }

    DataPage
    FileManager::create_page(
        const std::string& db_name,
        const std::string& schema_name,
        const std::string& table_id,
        const std::string& table_name
    )
    {
        DataPage page(make_uuid_str(), table_id, 0, 0, 0);
        save_page(db_name, schema_name, table_name, page);
        return page;
    }

    DataPage
    FileManager::load_page(
        const std::string& db_name,
        const std::string& schema_name,
        const std::string& table_name,
        const std::string& page_id
    ) const
    {
        auto path = path_db_schema_table_page(data_dir_, db_name, schema_name, table_name, page_id);

        if (!fs::exists(path))
            throw std::runtime_error("Data page " + page_id + " was not found");

        bytes_v bytes = read_file(path);

        DataPage page;
        if (!DataPage::try_deserialize(bytes, page))
            throw std::runtime_error("A content of the page " + page_id + " is corrupted");

        return page;
    }

    void
    FileManager::save_page(
        const std::string& db_name,
        const std::string& schema_name,
        const std::string& table_name,
        const DataPage& page
    )
    {
        auto path =
            path_db_schema_table_page(data_dir_, db_name, schema_name, table_name, page.id());
        bytes_v data = page.serialize();
        write_file(path, data);
    }

    std::optional<fs::path>
    FileManager::find_table_path(const std::string& db_name, const std::string& table_id) const
    {
        auto db_path = path_db(data_dir_, db_name);

        for (const auto& schema_entry : fs::directory_iterator(db_path))
        {
            if (!schema_entry.is_directory())
                continue;

            for (const auto& table_entry : fs::directory_iterator(schema_entry.path()))
            {
                if (!table_entry.is_directory())
                    continue;

                for (const auto& table_dir_files_entry : fs::directory_iterator(table_entry.path()))
                {
                    if (!table_dir_files_entry.is_regular_file())
                        continue;

                    auto path = table_dir_files_entry.path().stem();
                    auto stem = path.stem();

                    if (stem != table_entry.path().stem())
                        continue;

                    bytes_v content = read_file(table_dir_files_entry.path());
                    MetaTable table;
                    if (!MetaTable::try_deserialize(content, table))
                        continue;

                    if (table.id == table_id)
                        return table_dir_files_entry.path();
                }
            }
        }

        return std::nullopt;
    }

    bool
    FileManager::table_exists(const std::string& db_name, const std::string& table_id) const
    {
        return find_table_path(db_name, table_id) != std::nullopt;
    }

    bool
    FileManager::table_exists(
        const std::string& db_name,
        const std::string& schema_name,
        const std::string& table_name
    ) const
    {
        auto path = path_db_schema_table_meta(data_dir_, db_name, schema_name, table_name);
        return fs::exists(path);
    }

    MetaTable
    FileManager::load_table(const std::string& db_name, const std::string& table_id) const
    {
        auto path = find_table_path(db_name, table_id);

        if (path == std::nullopt)
        {
            throw std::runtime_error(
                "FileManager::load_table: table " + table_id + "doesnt exists in db " + db_name
            );
        }

        bytes_v bytes = read_file(path.value());
        MetaTable table;
        if (!MetaTable::try_deserialize(bytes, table))
            throw std::runtime_error(
                "FileManager::load_table: failed to deserialize table " + table_id +
                ": data are corrupted"
            );

        return table;
    }

    MetaTable
    FileManager::load_table(
        const std::string& db_name,
        const std::string& schema_name,
        const std::string& table_name
    ) const
    {
        auto path = path_db_schema_table_meta(data_dir_, db_name, schema_name, table_name);
        if (!fs::exists(path))
            throw std::runtime_error(
                "Metafile of the table " + schema_name + "." + table_name + " doesnt exist");

        auto content = read_file(path);

        MetaTable table;
        if (!MetaTable::try_deserialize(content, table))
            throw std::runtime_error(
                "Metafile of the table " + schema_name + "." + table_name + " is corrupted");

        return table;
    }

    void
    FileManager::save_table(
        const std::string& db_name,
        const std::string& schema_name,
        const MetaTable& table
    )
    {
        auto path = path_db_schema_table_meta(data_dir_, db_name, schema_name, table.name);
        bytes_v content = table.serialize();
        write_file(path, content);
    }

    std::optional<fs::path>
    FileManager::find_schema_path(const std::string& db_name, const std::string& schema_id) const
    {
        auto path = path_db(data_dir_, db_name);

        for (const auto& schema_entry : fs::directory_iterator(path))
        {
            if (!schema_entry.is_regular_file())
                continue;

            for (const auto& schema_dir_files_entry : fs::directory_iterator(schema_entry.path()))
            {
                if (!schema_dir_files_entry.is_regular_file())
                    continue;

                auto path = schema_dir_files_entry.path();
                auto stem = path.stem();

                if (stem != schema_entry.path().stem())
                    continue;

                bytes_v content = read_file(path);
                MetaSchema schema;
                if (!MetaSchema::try_deserialize(content, schema))
                    continue;

                if (schema.id == schema_id)
                    return path;
            }
        }

        return std::nullopt;
    }

    bool
    FileManager::schema_exists_by_id(const std::string& db_name, const std::string& schema_id) const
    {
        return find_schema_path(db_name, schema_id) != std::nullopt;
    }

    bool
    FileManager::schema_exists_by_name(
        const std::string& db_name,
        const std::string& schema_name
    ) const
    {
        auto path = path_db_schema_meta(data_dir_, db_name, schema_name);
        return fs::exists(path);
    }

    MetaSchema
    FileManager::load_schema_by_id(const std::string& db_name, const std::string& schema_id) const
    {
        auto path = find_schema_path(db_name, schema_id);
        if (path == std::nullopt)
            throw std::runtime_error(
                "FileManager::load_schema_by_id: failed to find a schema with id " + schema_id
            );

        bytes_v content = read_file(path.value());

        MetaSchema schema;
        if (!MetaSchema::try_deserialize(content, schema))
            throw std::runtime_error(
                "FileManager::load_schema_by_name: Metafile of the schema " + schema_id +
                " is corrupted"
            );

        return schema;
    }

    MetaSchema
    FileManager::load_schema_by_name(
        const std::string& db_name,
        const std::string& schema_name
    ) const
    {
        auto path = path_db_schema_meta(data_dir_, db_name, schema_name);
        if (!fs::exists(path))
            throw std::runtime_error(
                "FileManager::load_schema_by_name: Metafile of the schema " + schema_name +
                " doesnt exist"
            );

        auto content = read_file(path);

        MetaSchema schema;
        if (!MetaSchema::try_deserialize(content, schema))
            throw std::runtime_error(
                "FileManager::load_schema_by_name: Metafile of the schema " + schema_name +
                " is corrupted"
            );

        return schema;
    }

    void
    FileManager::save_schema(const MetaSchema& schema)
    {
        auto path = path_db_schema_meta(data_dir_, schema.db_name, schema.name);
        auto content = schema.serialize();
        write_file(path, content);
    }

    std::vector<fs::path>
    FileManager::get_tables_paths(const std::string& db_name, const std::string& schema_name) const
    {
        auto path = path_db_schema(data_dir_, db_name, schema_name);
        std::vector<fs::path> tables_dirs_paths;

        for (const auto& entry : fs::directory_iterator(path))
        {
            if (entry.is_directory())
            {
                tables_dirs_paths.push_back(entry.path());
            }
        }

        std::vector<fs::path> result(tables_dirs_paths.size());

        for (const auto& table_path : tables_dirs_paths)
        {
            std::string table_name = table_path.filename();
            result.emplace_back(table_path / make_meta_filename(table_name));
        }

        return result;
    }

    std::vector<fs::path>
    FileManager::get_schemata_paths(const std::string& db_name) const
    {
        const auto path = path_db(data_dir_, db_name);
        std::vector<fs::path> schemata_dirs_path;

        for (const auto& entry : fs::directory_iterator(path))
        {
            if (entry.is_directory())
            {
                schemata_dirs_path.push_back(entry.path());
            }
        }

        std::vector<fs::path> result(schemata_dirs_path.size());

        for (const auto& schema_path : schemata_dirs_path)
        {
            std::string schema_name = schema_path.filename();
            result.emplace_back(schema_path / make_meta_filename(schema_name));
        }

        return result;
    }

    bool
    FileManager::create_dir(const fs::path& path)
    {
        return fs::create_directory(path);
    }

    std::pair<std::ofstream, fs::path>
    FileManager::create_wal_logfile(
        const std::string& db_name,
        uint64_t first_lsn,
        uint64_t last_lsn
    ) const
    {
        auto path = path_db_wal_logfile(data_dir_, db_name, first_lsn, last_lsn);
        return std::make_pair(std::ofstream(path), path);
    }

    std::vector<MetaTable>
    FileManager::load_all_tables(const std::string& db_name) const
    {
        std::vector<MetaTable> tables;
        auto path = path_db(data_dir_, db_name);

        for (const auto& schema_dir : fs::directory_iterator(path))
        {
            if (!schema_dir.is_directory())
                continue;

            for (const auto& table_dir : fs::directory_iterator(schema_dir.path()))
            {
                if (!table_dir.is_directory())
                    continue;

                tables.emplace_back(
                    load_table(db_name,
                               schema_dir.path().filename(),
                               table_dir.path().filename()
                    )
                );
            }
        }

        return tables;
    }

    std::vector<MetaSchema>
    FileManager::load_all_schemas(const std::string& db_name) const
    {
        std::vector<MetaSchema> schemas;
        auto path = path_db(data_dir_, db_name);

        for (const auto& schema_dir : fs::directory_iterator(path))
        {
            if (!schema_dir.is_directory())
                continue;

            schemas.emplace_back(load_schema_by_name(db_name, schema_dir.path().filename()));
        }

        return schemas;
    }

    std::vector<DataPage>
    FileManager::load_all_pages(const std::string& db_name) const
    {
        auto db_path = path_db(data_dir_, db_name);
        std::vector<DataPage> result;

        for (const auto& schema_dir : fs::directory_iterator(db_path))
        {
            if (!schema_dir.is_directory())
                continue;

            for (const auto& table_dir : fs::directory_iterator(schema_dir.path()))
            {
                if (!table_dir.is_directory())
                    continue;

                auto data_path = path_db_schema_table_data(
                    data_dir_,
                    db_name,
                    schema_dir.path().filename(),
                    table_dir.path().filename()
                );

                for (const auto& page_file : fs::directory_iterator(data_path))
                {
                    if (!page_file.is_regular_file())
                        continue;

                    auto content = read_file(page_file.path());
                    DataPage page;
                    if (!DataPage::try_deserialize(content, page))
                        throw std::runtime_error(
                            "FileManager::load_all_pages: failed to serialize data page");

                    result.push_back(std::move(page));
                }
            }
        }

        return result;
    }

    std::vector<WalLogfile>
    FileManager::load_wal(const std::string& db_name) const
    {
        auto wal_path = path_db_wal(data_dir_, db_name);
        std::vector<WalLogfile> log;

        for (const auto& entry : fs::directory_iterator(wal_path))
        {
            if (!entry.is_regular_file())
                continue;

            bytes_v content = read_file(entry.path());
            WalLogfile logfile;
            if (!WalLogfile::try_deserialize(content, logfile))
                throw std::runtime_error(
                    "FileManager::load_wal: failed to deserialize wal logfile");

            log.push_back(std::move(logfile));
        }

        return log;
    }
} // namespace storage