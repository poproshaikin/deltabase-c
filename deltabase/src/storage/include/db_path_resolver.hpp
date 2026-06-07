//
// Created by poproshaikin on 07.06.26.
//

#ifndef DELTABASE_DB_PATH_RESOLVER_HPP
#define DELTABASE_DB_PATH_RESOLVER_HPP

#include "path.hpp"

#include <filesystem>
#include <string>

namespace storage
{
    namespace fs = std::filesystem;

    // Single source of truth for all filesystem paths of a database instance.
    //
    // Layout enforced by this class:
    //   {root}/{db}/
    //   {root}/{db}/{db}.meta
    //   {root}/{db}/wal/
    //   {root}/{db}/{schema}/
    //   {root}/{db}/{schema}/{schema}.meta
    //   {root}/{db}/{schema}/tables/{table}/
    //   {root}/{db}/{schema}/tables/{table}/{table}.meta
    //   {root}/{db}/{schema}/tables/{table}/data/{page_id}
    //   {root}/{db}/{schema}/tables/{table}/index/{index_id}
    //   {root}/{db}/{schema}/sequences/{seq_name}
    class DbPathResolver
    {
        fs::path root_;
        std::string db_;

    public:
        DbPathResolver() = default;

        DbPathResolver(fs::path root, std::string db_name)
            : root_(std::move(root)), db_(std::move(db_name))
        {}

        fs::path db() const
        {
            return root_ / db_;
        }

        fs::path db_meta() const
        {
            return db() / make_meta_filename(db_);
        }

        fs::path wal() const
        {
            return db() / PATH_WAL;
        }

        fs::path schema(const std::string& schema_name) const
        {
            return db() / schema_name;
        }

        fs::path schema_meta(const std::string& schema_name) const
        {
            return schema(schema_name) / make_meta_filename(schema_name);
        }

        fs::path tables_dir(const std::string& schema_name) const
        {
            return schema(schema_name) / PATH_TABLES;
        }

        fs::path table(const std::string& schema_name, const std::string& table_name) const
        {
            return tables_dir(schema_name) / table_name;
        }

        fs::path table_meta(const std::string& schema_name, const std::string& table_name) const
        {
            return table(schema_name, table_name) / make_meta_filename(table_name);
        }

        fs::path table_data_dir(const std::string& schema_name, const std::string& table_name) const
        {
            return table(schema_name, table_name) / PATH_DATA;
        }

        fs::path table_page(
            const std::string& schema_name,
            const std::string& table_name,
            const std::string& page_id
        ) const
        {
            return table_data_dir(schema_name, table_name) / page_id;
        }

        fs::path table_index_dir(const std::string& schema_name, const std::string& table_name) const
        {
            return table(schema_name, table_name) / PATH_INDEX;
        }

        fs::path table_index(
            const std::string& schema_name,
            const std::string& table_name,
            const std::string& index_id
        ) const
        {
            return table_index_dir(schema_name, table_name) / index_id;
        }

        fs::path sequences_dir(const std::string& schema_name) const
        {
            return schema(schema_name) / PATH_SEQUENCES;
        }

        fs::path sequence(const std::string& schema_name, const std::string& seq_name) const
        {
            return sequences_dir(schema_name) / seq_name;
        }
    };

} // namespace storage

#endif //DELTABASE_DB_PATH_RESOLVER_HPP
