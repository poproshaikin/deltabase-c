//
// Created by poproshaikin on 6/11/26.
//

#ifndef DELTABASE_DML_SERVICE_HPP
#define DELTABASE_DML_SERVICE_HPP
#include "ddl_service.hpp"
#include "../../types/include/data_token.hpp"
#include "../../transactions/include/transaction.hpp"

#include <optional>
#include <string>
#include <vector>

namespace storage
{
    class DMLService
    {
        DDLService ddl_service;
        BufferPool& buffer_pool;



        std::vector<types::IndexId>
        insert_row_into_indexes(
            const types::MetaTable& mt, const types::DataRow& row, const types::DataPageId& page_id);

    public:
        void
        insert_row(
            const std::string& table_name,
            const std::string& schema_name,
            const std::optional<std::vector<std::string>>& cols,
            std::vector<types::DataToken> row,
            txn::Transaction& txn);

        void
        update_sequentially(
            const std::string& table_name,
            const std::string& schema_name,
            types::RowUpdate update,
            const std::vector<types::DataRow>& rows,
            txn::Transaction& txn);

        void
        delete_sequentially(
            const std::string& table_name,
            const std::string& schema_name,
            const std::vector<types::DataRow>& rows,
            txn::Transaction& txn);

    };
}

#endif //DELTABASE_DML_SERVICE_HPP
