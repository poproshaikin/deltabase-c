#include "include/file_manager.hpp"
#include "include/objects/meta_object.hpp"
#include "include/paths.hpp"
#include <bits/fs_fwd.h>
#include <bits/fs_path.h>
#include <complex.h>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace storage {

    FileManager::FileManager(const fs::path& data_dir) : data_dir_(data_dir) {
        if (!fs::exists(data_dir_)) {
            fs::create_directories(data_dir_);
        }
    }

    bytes_v
    FileManager::read_file(const fs::path& path) const {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Cannot open file: " + path.string());
        }

        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);

        bytes_v data(size);
        file.read(reinterpret_cast<char*>(data.data()), size);

        if (!file) {
            throw std::runtime_error("Error reading file: " + path.string());
        }

        return data;
    }

    void
    FileManager::write_file(const fs::path& path, const bytes_v& content) {
        if (path.has_parent_path()) {
            fs::create_directories(path.parent_path());
        }

        std::ofstream file(path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Cannot create file: " + path.string());
        }

        file.write(reinterpret_cast<const char*>(content.data()), content.size());
        if (!file) {
            throw std::runtime_error("Error writing file: " + path.string());
        }
    }

    std::optional<fs::path>
    FileManager::find_page_path(const std::string& db_name, const std::string& page_id) const {
        auto db_path = path_db(data_dir_, db_name);

        for (const auto& schema_entry : fs::directory_iterator(db_path)) {
            if (!schema_entry.is_directory())
                continue;

            for (const auto& table_entry : fs::directory_iterator(schema_entry.path())) {
                if (!table_entry.is_directory())
                    continue;

                auto table_data_path = path_db_schema_table_data(
                    data_dir_,
                    db_name,
                    schema_entry.path().filename(),
                    table_entry.path().filename()
                );

                for (const auto& page_entry : fs::directory_iterator(table_data_path)) {
                    if (!page_entry.is_regular_file())
                        continue;

                    if (page_entry.path().filename() == page_id) {
                        return page_entry.path();
                    }
                }
            }
        }

        return std::nullopt;
    }

    bool
    FileManager::page_exists(const std::string& db_name, const std::string& page_id) const {
        return find_page_path(db_name, page_id) != std::nullopt;
    }

    DataPage
    FileManager::load_page(const std::string& db_name, const std::string& page_id) const {
        auto path = find_page_path(db_name, page_id);
        
        if (path == std::nullopt) {
            throw std::runtime_error("FileManager::load_page: page " + page_id + "doesnt exists in db " + db_name);
        }

        bytes_v bytes = read_file(path.value());
        if (!DataPage::can_deserialize(bytes)) {
            throw std::runtime_error("FileManager::load_page: failed to deserialize page " + page_id + ": data are corrupted");
        }

        return DataPage::deserialize(bytes);
    }

    bool
    FileManager::exists_page(
        const std::string& db_name,
        const std::string& schema_name,
        const std::string& table_name,
        const std::string& page_id
    ) const {
        auto path = path_db_schema_table_page(data_dir_, db_name, schema_name, table_name, page_id);
        return fs::exists(path);
    }

    DataPage
    FileManager::load_page(
        const std::string& db_name,
        const std::string& schema_name,
        const std::string& table_name,
        const std::string& page_id
    ) const {
        auto path = path_db_schema_table_page(data_dir_, db_name, schema_name, table_name, page_id);

        if (!fs::exists(path)) {
            throw std::runtime_error("Data page " + page_id + " was not found");
        }

        bytes_v bytes = read_file(path);
        if (!DataPage::can_deserialize(bytes)) {
            throw std::runtime_error("A content of the page " + page_id + " is corrupted");
        }

        return DataPage::deserialize(bytes);
    }

    void
    FileManager::save_page(
        const std::string& db_name,
        const std::string& schema_name,
        const std::string& table_name,
        const DataPage& page
    ) {
        auto path =
            path_db_schema_table_page(data_dir_, db_name, schema_name, table_name, page.id());
        bytes_v data = page.serialize();
        write_file(path, data);
    }

    std::optional<fs::path>
    FileManager::find_table_path(const std::string& db_name, const std::string& table_id) const {
        auto db_path = path_db(data_dir_, db_name);

        for (const auto& schema_entry : fs::directory_iterator(db_path)) {
            if (!schema_entry.is_directory())
                continue;

            for (const auto& table_entry : fs::directory_iterator(schema_entry.path())) {
                if (!table_entry.is_directory())
                    continue;

                for (const auto& table_dir_files_entry : fs::directory_iterator(table_entry.path())) {
                    if (!table_dir_files_entry.is_regular_file())
                        continue;

                    auto path = table_dir_files_entry.path().stem();
                    auto stem = path.stem();

                    if (stem != table_entry.path().stem()) 
                        continue;

                    bytes_v content = read_file(table_dir_files_entry.path());
                    if (!MetaTable::can_deserialize(content)) 
                        continue;
                    
                    auto table = MetaTable::deserialize(content);
                    if (table.id == table_id) {
                        return table_dir_files_entry.path();
                    }
                }
            }
        }

        return std::nullopt;
    }

    bool
    FileManager::table_exists(const std::string& db_name, const std::string& table_id) const {
        return find_table_path(db_name, table_id) != std::nullopt;
    }

    bool
    FileManager::table_exists(
        const std::string& db_name, const std::string& schema_name, const std::string& table_name
    ) const {
        auto path = path_db_schema_table_meta(data_dir_, db_name, schema_name, table_name);
        return fs::exists(path);
    }

    MetaTable
    FileManager::load_table(const std::string& db_name, const std::string& table_id) const {
        auto path = find_table_path(db_name, table_id);
        
        if (path == std::nullopt) {
            throw std::runtime_error("FileManager::load_table: table " + table_id + "doesnt exists in db " + db_name);
        }

        bytes_v bytes = read_file(path.value());
        if (!DataPage::can_deserialize(bytes)) {
            throw std::runtime_error("FileManager::load_table: failed to deserialize table " + table_id + ": data are corrupted");
        }

        return MetaTable::deserialize(bytes);
    }

    MetaTable
    FileManager::load_table(
        const std::string& db_name, const std::string& schema_name, const std::string& table_name
    ) const {
        auto path = path_db_schema_table_meta(data_dir_, db_name, schema_name, table_name);
        if (!fs::exists(path)) {
            throw std::runtime_error("Metafile of the table " + schema_name + "." + table_name + " doesnt exist");
        }

        auto data = read_file(path);

        if (!MetaTable::can_deserialize(data)) {
            throw std::runtime_error("Metafile of the table " + schema_name + "." + table_name + " is corrupted");
        }

        return MetaTable::deserialize(data);
    }

    void
    FileManager::save_table(
        const std::string& db_name,
        const std::string& schema_name,
        const MetaTable& table
    ) {
        auto path = path_db_schema_table_meta(data_dir_, db_name, schema_name, table.name);
        bytes_v data = table.serialize();
        write_file(path, data);
    }

    std::optional<fs::path>
    FileManager::find_schema_path(const std::string& db_name, const std::string& schema_id) const {
        auto path = path_db(data_dir_, db_name);

        for (const auto& schema_entry : fs::directory_iterator(path)) {
            if (!schema_entry.is_regular_file())
                continue;
            
            for (const auto& schema_dir_files_entry : fs::directory_iterator(schema_entry.path())) {
                if (!schema_dir_files_entry.is_regular_file()) 
                    continue;

                auto path = schema_dir_files_entry.path();
                auto stem = path.stem();

                if (stem != schema_entry.path().stem()) 
                    continue;

                bytes_v content = read_file(path);
                if (!MetaSchema::can_deserialize(content)) 
                    continue;

                auto schema = MetaSchema::deserialize(content);

                if (schema.id == schema_id) 
                    return path;
            }
        }

        return std::nullopt;
    }

    bool
    FileManager::schema_exists_by_id(const std::string& db_name, const std::string& schema_id) const {
        return find_schema_path(db_name, schema_id) != std::nullopt;
    }

    bool
    FileManager::schema_exists_by_name(const std::string& db_name, const std::string& schema_name) const {
        auto path = path_db_schema_meta(data_dir_, db_name, schema_name);
        return fs::exists(path);
    }

    MetaSchema
    FileManager::load_schema_by_id(const std::string& db_name, const std::string& schema_id) const {
        auto path = find_schema_path(db_name, schema_id);
        if (path == std::nullopt) 
            throw std::runtime_error("FileManager::load_schema_by_id: failed to find a schema with id " + schema_id);

        bytes_v content = read_file(path.value());

        if (!MetaSchema::can_deserialize(content))
            throw std::runtime_error("FileManager::load_schema_by_name: Metafile of the schema " + schema_id + " is corrupted");

        return MetaSchema::deserialize(content);
    }

    MetaSchema
    FileManager::load_schema_by_name(const std::string& db_name, const std::string& schema_name) const {
        auto path = path_db_schema_meta(data_dir_, db_name, schema_name);
        if (!fs::exists(path)) 
            throw std::runtime_error("FileManager::load_schema_by_name: Metafile of the schema " + schema_name + " doesnt exist");

        auto data = read_file(path);

        if (!MetaSchema::can_deserialize(data)) 
            throw std::runtime_error("FileManager::load_schema_by_name: Metafile of the schema " + schema_name + " is corrupted");

        return MetaSchema::deserialize(data);
    }

    void
    FileManager::save_schema(const MetaSchema& schema) {
        auto path = path_db_schema_meta(data_dir_, schema.db_name, schema.name);
        auto data = schema.serialize();
        write_file(path, data);
    }

    std::vector<fs::path>
    FileManager::get_tables_paths(const std::string& db_name, const std::string& schema_name) {
        auto path = path_db_schema(data_dir_, db_name, schema_name);
        std::vector<fs::path> tables_dirs_paths;

        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_directory()) {
                tables_dirs_paths.push_back(entry.path());
            }
        }

        std::vector<fs::path> result(tables_dirs_paths.size());

        for (const auto& path : tables_dirs_paths) {
            std::string table_name = path.filename();
            result.emplace_back(path / make_meta_filename(table_name));
        }

        return result;
    }

    std::vector<fs::path>
    FileManager::get_schemata_paths(const std::string& db_name) {
        auto path = path_db(data_dir_, db_name);
        std::vector<fs::path> schemata_dirs_path;

        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_directory()) {
                schemata_dirs_path.push_back(entry.path());
            }
        }

        std::vector<fs::path> result(schemata_dirs_path.size()) ;
        
        for (const auto& path : schemata_dirs_path) {
            std::string schema_name = path.filename();
            result.emplace_back(path / make_meta_filename(schema_name));
        }

        return result;
    }
} // namespace storage