#pragma once 

#include <cstdint>
#include <string>
#include <filesystem>


// data/
// data/db_name/
// data/db_name/schema_name/
// data/db_name/schema_name/schema_name.meta
// data/db_name/schema_name/table_name/
// data/db_name/schema_name/table_name/table_name.meta   
// data/db_name/schema_name/table_name/data/ 
// data/db_name/schema_name/table_name/data/2093ru20rj2039j2f29jf209fej <-> page (name is page_id)
// data/db_name/wal/
// data/db_name/wal/wal_logfile

namespace storage
{
    namespace fs = std::filesystem;

    static const std::string data = "data";
    static const std::string wal  = "wal";
    static const std::string meta = "meta";

    fs::path
    path_db_wal(const fs::path& data_dir, const std::string db_name)
    {
        return data_dir / db_name / wal;
    }

    fs::path
    path_db_wal_logfile(
        const fs::path& data_dir, const std::string& db_name, uint64_t first_lsn, uint64_t last_lsn
    )
    {
        return data_dir / db_name / (std::to_string(first_lsn) + "_" + std::to_string(last_lsn));
    }

    fs::path
    path_db(const fs::path& data_dir, const std::string& db_name)
    {
        return data_dir / db_name;
    }

    fs::path
    path_db_schema_table_page(
        const fs::path& data_dir,
        const std::string& db_name,
        const std::string& schema_name,
        const std::string& table_name,
        const std::string& page_id
    )
    {
        return data_dir / db_name / schema_name / table_name / data / page_id;
    }

    fs::path
    path_db_schema_table(
        const fs::path& data_dir,
        const std::string& db_name,
        const std::string& schema_name,
        const std::string& table_name
    );

    fs::path
    path_db_schema_table_data(
        const fs::path& data_dir,
        const std::string& db_name,
        const std::string& schema_name,
        const std::string& table_name
    );

    fs::path
    path_db_schema_table_meta(
        const fs::path& data_dir,
        const std::string& db_name,
        const std::string& schema_name,
        const std::string& table_name
    );

    fs::path
    path_db_schema(
        const fs::path& data_dir, const std::string& db_name, const std::string& schema_name
    );

    fs::path
    path_db_schema_meta(
        const fs::path& data_dir, const std::string& db_name, const std::string& schema_name
    );

    inline std::string
    make_meta_filename(const std::string& name)
    {
        return name + "." + meta;
    }
} // namespace storage