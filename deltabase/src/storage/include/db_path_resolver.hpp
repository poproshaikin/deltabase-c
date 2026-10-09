//
// Created by poproshaikin on 07.06.26.
//

#ifndef DELTABASE_DB_PATH_RESOLVER_HPP
#define DELTABASE_DB_PATH_RESOLVER_HPP

#include "path.hpp"
#include "../../types/include/UUID.hpp"

#include <filesystem>
#include <string>

namespace storage
{
    namespace fs = std::filesystem;

    // Single source of truth for all filesystem paths of a database instance.
    //
    // Schemas, tables and sequences are identified on disk by their stable id,
    // never by their (renameable) name -- the name only ever lives inside the
    // entity's own meta content, deserialized at read time. This means a SQL
    // rename never has to touch the filesystem.
    //
    // Layout enforced by this class:
    //   {root}/{db}/
    //   {root}/{db}/{db}.meta
    //   {root}/{db}/wal/
    //   {root}/{db}/{schema_id}/
    //   {root}/{db}/{schema_id}/meta
    //   {root}/{db}/{schema_id}/tables/{table_id}/
    //   {root}/{db}/{schema_id}/tables/{table_id}/meta
    //   {root}/{db}/{schema_id}/tables/{table_id}/data/{page_id}
    //   {root}/{db}/{schema_id}/tables/{table_id}/index/{index_id}
    //   {root}/{db}/{schema_id}/sequences/{seq_id}
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

        fs::path control_file() const
        {
            return db() / "control";
        }

        fs::path schema(const types::UUID& schema_id) const
        {
            return db() / schema_id.to_string();
        }

        fs::path schema_meta(const types::UUID& schema_id) const
        {
            return schema(schema_id) / PATH_META;
        }

        fs::path tables_dir(const types::UUID& schema_id) const
        {
            return schema(schema_id) / PATH_TABLES;
        }

        fs::path table(const types::UUID& schema_id, const types::UUID& table_id) const
        {
            return tables_dir(schema_id) / table_id.to_string();
        }

        fs::path table_meta(const types::UUID& schema_id, const types::UUID& table_id) const
        {
            return table(schema_id, table_id) / PATH_META;
        }

        fs::path table_data_dir(const types::UUID& schema_id, const types::UUID& table_id) const
        {
            return table(schema_id, table_id) / PATH_DATA;
        }

        fs::path table_page(
            const types::UUID& schema_id,
            const types::UUID& table_id,
            const std::string& page_id
        ) const
        {
            return table_data_dir(schema_id, table_id) / page_id;
        }

        fs::path table_index_dir(const types::UUID& schema_id, const types::UUID& table_id) const
        {
            return table(schema_id, table_id) / PATH_INDEX;
        }

        fs::path table_index(
            const types::UUID& schema_id,
            const types::UUID& table_id,
            const std::string& index_id
        ) const
        {
            return table_index_dir(schema_id, table_id) / index_id;
        }

        fs::path sequences_dir(const types::UUID& schema_id) const
        {
            return schema(schema_id) / PATH_SEQUENCES;
        }

        fs::path sequence(const types::UUID& schema_id, const types::UUID& seq_id) const
        {
            return sequences_dir(schema_id) / seq_id.to_string();
        }
    };

} // namespace storage

#endif //DELTABASE_DB_PATH_RESOLVER_HPP
