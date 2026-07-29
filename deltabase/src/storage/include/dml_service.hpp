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
        DDLService& ddl_service_;
        BufferPool& buffer_pool_;
        IIOManager& io_manager_;

        std::vector<types::IndexId>
        insert_row_into_indexes(
            const types::MetaTable& mt, const types::DataRow& row, const types::DataPageId& page_id);

        bool
        is_row_obsolete(const types::RowPtr& row_ptr) const;

    public:
        explicit DMLService(
            DDLService& ddl_service,
            BufferPool& buffer_pool,
            IIOManager& io_manager);

        void
        insert_row(
            types::MetaTable& mt,
            std::vector<types::DataToken> normalized_row,
            txn::Transaction& txn);

        void
        update_selected(
            types::MetaTable& mt,
            types::RowUpdate update,
            const std::vector<types::DataRow>& rows,
            txn::Transaction& txn);

        void
        delete_selected(
            types::MetaTable& mt,
            const std::vector<types::DataRow>& rows,
            txn::Transaction& txn);

    };
}

#endif //DELTABASE_DML_SERVICE_HPP
