//
// Created by poproshaikin on 11.11.25.
//

#ifndef DELTABASE_STORAGE_HPP
#define DELTABASE_STORAGE_HPP
#include "../../transactions/include/transaction.hpp"
#include "../../types/include/config.hpp"
#include "../../types/include/data_row.hpp"
#include "../../types/include/data_table.hpp"
#include "../../types/include/meta_schema.hpp"
#include "../../types/include/query_plan.hpp"

namespace storage
{
    class IDbInstance
    {
    public:
        virtual ~IDbInstance() = default;

        virtual bool
        needs_stream(types::IPlanNode& plan_node) = 0;

        virtual types::DataTable
        seq_scan(const std::string& table_name, const std::string& schema_name) = 0;

        virtual types::DataTable
        index_scan(
            const std::string& table_name,
            const std::string& schema_name,
            const types::IndexId& index_id,
            const types::BinaryExpr& condition
        ) = 0;

        virtual txn::Transaction
        make_txn() = 0;

        virtual void
        insert_row(
            const std::string& table_name,
            const std::string& schema_name,
            const std::optional<std::vector<std::string>>& cols,
            std::vector<types::DataToken> rows,
            txn::Transaction& txn
        ) = 0;

        virtual void
        insert_row_into_indexes(
            const types::MetaTable& mt, const types::DataRow& row, const types::DataPageId& page_id
        ) = 0;

        virtual void
        update_row(
            const std::string& table_name,
            const std::string& schema_name,
            types::RowUpdate update,
            const std::vector<types::DataRow>& rows,
            txn::Transaction& txn
        ) = 0;

        virtual void
        delete_rows(
            const std::string& table_name,
            const std::string& schema_name,
            const std::vector<types::DataRow>& rows,
            txn::Transaction& txn
        ) = 0;

        virtual bool
        exists_table(const std::string& table_name, const std::string& schema_name) = 0;

        virtual bool
        exists_table(const types::TableIdentifier& identifier) = 0;

        virtual types::MetaTable*
        get_table(const std::string& table_name, const std::string& schema_name) = 0;

        virtual types::MetaTable*
        get_table(const types::TableIdentifier& identifier) = 0;

        virtual types::MetaSchema*
        get_schema(const std::string& name) = 0;

        virtual const types::Config&
        get_config() const = 0;

        virtual bool
        exists_db(const std::string& value) = 0;

        virtual void
        create_table(
            const std::string& table_name,
            const std::string& schema_name,
            const std::vector<types::ColumnDefinition>& vector,
            txn::Transaction& txn
        ) = 0;

        virtual void
        create_schema(const std::string& schema_name, txn::Transaction& txn) = 0;

        virtual bool
        exists_schema(const std::string& schema_name) = 0;

        virtual void
        create_index(
            const std::string& index_name,
            const std::string& table_name,
            const std::string& column_name,
            const std::string& schema_name,
            bool is_unique,
            txn::Transaction& txn
        ) = 0;

        virtual bool
        exists_index(
            const std::string& index_name,
            const std::string& table_name,
            const std::string& schema_name
        ) = 0;

        virtual bool
        exists_index(
            const std::string& index_name, const types::TableIdentifier& table_identifier
        ) = 0;

        virtual types::MetaIndex*
        get_index(
            const std::string& index_name,
            const std::string& table_name,
            const std::string& schema_name
        ) = 0;

        virtual types::MetaIndex*
        get_index(
            const std::string& index_name, const types::TableIdentifier& table_identifier
        ) = 0;

        virtual void
        drop_index(
            const std::string& index_name,
            const std::string& table_name,
            const std::string& schema_name,
            txn::Transaction& txn
        ) = 0;
    };
} // namespace storage

#endif // DELTABASE_STORAGE_HPP