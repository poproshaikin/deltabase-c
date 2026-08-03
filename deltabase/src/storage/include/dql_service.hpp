//
// Created by poproshaikin on 6/19/26.
//

#ifndef DELTABASE_DQL_SERVICE_HPP
#define DELTABASE_DQL_SERVICE_HPP

#include "buffer_pool.hpp"
#include "../../types/include/data_row.hpp"
#include "../../types/include/data_table.hpp"
#include "../../types/include/meta_table.hpp"
#include "../../types/include/ast_tree.hpp"
#include "scan_cursor.hpp"

namespace storage
{
    class DQLService
    {
        BufferPool& buffer_pool_;

    public:
        explicit DQLService(BufferPool& buffer_pool);

        types::DataTable
        seq_scan(const types::MetaTable& mt);

        types::ScanCursor
        seq_scan_begin(const types::MetaTable& mt);

        bool
        seq_scan_next(types::ScanCursor& cursor, types::DataRow& out);

        types::DataTable
        index_scan(
            const types::MetaTable& mt,
            const types::IndexId& index_id,
            const types::BinaryExpr& condition);

        bool
        value_exists(
            const types::MetaTable& mt,
            const std::string& column_name,
            const types::DataToken& value,
            std::optional<types::RowId> exclude = std::nullopt);

        std::vector<types::DataRow>
        get_rows_with_value(
            const types::MetaTable& mt,
            const std::string& col_name,
            const types::DataToken& token);
    };
}

#endif //DELTABASE_DQL_SERVICE_HPP
