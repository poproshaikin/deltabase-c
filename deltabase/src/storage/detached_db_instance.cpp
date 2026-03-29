//
// Created by poproshaikin on 15.01.26.
//

#include "detached_db_instance.hpp"

#include "io_manager_factory.hpp"

#include <math.h>

namespace storage
{
    using namespace types;

    DetachedDbInstance::DetachedDbInstance(const Config& config) : config_(config)
    {
        IOManagerFactory io_factory;
        io_manager_ = io_factory.make(config);
        io_manager_->init();
    }

    bool
    DetachedDbInstance::exists_db(const std::string& db_name)
    {
        return io_manager_->exists_db(db_name);
    }

    const Config&
    DetachedDbInstance::get_config() const
    {
        return config_;
    }

    bool
    DetachedDbInstance::needs_stream(types::IPlanNode& plan_node)
    {
        throw std::logic_error("DetachedDbInstance::needs_stream: this method is not supported");
    }

    DataTable
    DetachedDbInstance::seq_scan(const std::string& table_name, const std::string& schema_name)
    {
        throw std::logic_error("DetachedDbInstance::seq_scan: this method is not supported");
    }

    txn::Transaction
    DetachedDbInstance::make_txn()
    {
        throw std::logic_error("DetachedDbInstance::make_transaction: this method is not supported");
    }

    void
    DetachedDbInstance::insert_row(
        const std::string& table_name,
        const std::string& schema_name,
        std::vector<DataToken> row,
        txn::Transaction& txn
    )
    {
        throw std::logic_error("DetachedDbInstance::insert_row: this method is not supported");
    }

    bool
    DetachedDbInstance::exists_table(const std::string& table_name, const std::string& schema_name)
    {
        throw std::logic_error("DetachedDbInstance::exists_table: this method is not supported");
    }

    bool
    DetachedDbInstance::exists_table(const types::TableIdentifier& identifier)
    {
        throw std::logic_error("DetachedDbInstance::exists_table: this method is not supported");
    }

    MetaTable*
    DetachedDbInstance::get_table(const std::string& table_name, const std::string& schema_name)
    {
        throw std::logic_error("DetachedDbInstance::get_table: this method is not supported");
    }

    MetaTable*
    DetachedDbInstance::get_table(const TableIdentifier& identifier)
    {
        throw std::logic_error("DetachedDbInstance::get_table: this method is not supported");
    }

    MetaSchema*
    DetachedDbInstance::get_schema(const std::string& name)
    {
        throw std::logic_error("DetachedDbInstance::get_schema: this method is not supported");
    }

    void
    DetachedDbInstance::create_table(
        const std::string& table_name,
        const std::string& schema_name,
        const std::vector<ColumnDefinition>& vector,
        txn::Transaction& txn
    )
    {
        throw std::logic_error("DetachedDbInstance::create_table: this method is not supported");
    }

    void
    DetachedDbInstance::create_schema(const std::string& schema_name, txn::Transaction& txn)
    {
        throw std::logic_error("DetachedDbInstance::create_schema: this method is not supported");
    }

    bool
    DetachedDbInstance::exists_schema(const std::string& schema_name)
    {
        throw std::logic_error("DetachedDbInstance::exists_schema: this method is not supported");
    }

    void
    DetachedDbInstance::update_row(
        const std::string& table_name,
        const std::string& schema_name,
        types::RowUpdate update,
        const std::vector<types::DataRow>& rows,
        txn::Transaction& txn
    )
    {
        throw std::logic_error("DetachedDbInstance::update_row: this method is not supported");
    }
    void
    DetachedDbInstance::delete_rows(
        const std::string& table_name,
        const std::string& schema_name,
        const std::vector<types::DataRow>& rows,
        txn::Transaction& txn
    )
    {
        throw std::logic_error("DetachedDbInstance::delete_row: this method is not supported");
    }
} // namespace storage