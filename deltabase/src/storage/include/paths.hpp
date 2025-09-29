#pragma once 

#include <string>
#include <filesystem>


// data/
// data/db_name/
// data/db_name/schema_name
// data/db_name/schema_name.meta
// data/db_name/schema_name/table_name/
// data/db_name/schema_name/table_name/table_name.meta   
// data/db_name/schema_name/table_name/data/ 
// data/db_name/schema_name/table_name/data/2093ru20rj2039j2f29jf209fej <-> page (name is page_id)

namespace storage {
    namespace fs = std::filesystem;

    static const std::string DATA = "data";
    static const std::string WAL  = "wal";
    static const std::string META = "meta";

    fs::path
    path_db_wal(const fs::path& data_dir) {
        return data_dir / WAL;
    }

    fs::path
    path_db_wal() {
        return path_db_wal(DATA);
    }

    fs::path
    path_db(const fs::path& data_dir, const std::string& db_name) {
        return data_dir / db_name;
    }

    fs::path
    path_db_schema_table_page(
        const fs::path& data_dir,
        const std::string& db_name,
        const std::string& schema_name,
        const std::string& table_name,
        const std::string& page_id
    ) {
        return data_dir / db_name / schema_name / table_name / DATA / page_id;
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
    make_meta_filename(const std::string& name) {
        return name + "." + META;
    }
}