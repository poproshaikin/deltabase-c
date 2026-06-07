//
// Created by poproshaikin on 15.01.26.
//

#include "detached_db_instance.hpp"

#include "io_manager_factory.hpp"
#include "../../misc/include/exceptions.hpp"

namespace storage
{
    using namespace types;

    [[noreturn]] static void unsupported()
    {
        throw EngineException("no database attached", EngineException::Code::DB_NOT_ATTACHED);
    }

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

    DataTable
    DetachedDbInstance::seq_scan(const std::string&, const std::string&) { unsupported(); }

    ScanCursor
    DetachedDbInstance::seq_scan_begin(const std::string&, const std::string&) { unsupported(); }

    bool
    DetachedDbInstance::seq_scan_next(ScanCursor&, DataRow&) { unsupported(); }

    DataTable
    DetachedDbInstance::index_scan(
        const std::string&, const std::string&, const IndexId&, const BinaryExpr&
    ) { unsupported(); }

    txn::Transaction
    DetachedDbInstance::make_txn() { unsupported(); }

    void
    DetachedDbInstance::insert_row(
        const std::string&, const std::string&,
        const std::optional<std::vector<std::string>>&,
        std::vector<DataToken>, txn::Transaction&
    ) { unsupported(); }

    bool
    DetachedDbInstance::exists_table(const std::string&, const std::string&) { unsupported(); }

    bool
    DetachedDbInstance::exists_table(const TableIdentifier&) { unsupported(); }

    MetaTable*
    DetachedDbInstance::get_table(const std::string&, const std::string&) { unsupported(); }

    MetaTable*
    DetachedDbInstance::get_table(const TableIdentifier&) { unsupported(); }

    MetaSchema*
    DetachedDbInstance::get_schema(const std::string&) { unsupported(); }

    void
    DetachedDbInstance::create_table(
        const std::string&, const std::string&,
        const std::vector<ColumnDefinition>&, txn::Transaction&
    ) { unsupported(); }

    void
    DetachedDbInstance::create_schema(const std::string&, txn::Transaction&) { unsupported(); }

    bool
    DetachedDbInstance::exists_schema(const std::string&) { unsupported(); }

    void
    DetachedDbInstance::update_row(
        const std::string&, const std::string&,
        RowUpdate, const std::vector<DataRow>&, txn::Transaction&
    ) { unsupported(); }

    void
    DetachedDbInstance::delete_rows(
        const std::string&, const std::string&,
        const std::vector<DataRow>&, txn::Transaction&
    ) { unsupported(); }

    void
    DetachedDbInstance::create_index(
        const std::string&, const std::string&, const std::string&, const std::string&,
        bool, bool, txn::Transaction&
    ) { unsupported(); }

    std::vector<IndexId>
    DetachedDbInstance::insert_row_into_indexes(
        const MetaTable&, const DataRow&, const DataPageId&
    ) { unsupported(); }

    bool
    DetachedDbInstance::exists_index(
        const std::string&, const std::string&, const std::string&
    ) { unsupported(); }

    bool
    DetachedDbInstance::exists_index(const std::string&, const TableIdentifier&) { unsupported(); }

    MetaIndex*
    DetachedDbInstance::get_index(
        const std::string&, const std::string&, const std::string&
    ) { unsupported(); }

    MetaIndex*
    DetachedDbInstance::get_index(const std::string&, const TableIdentifier&) { unsupported(); }

    void
    DetachedDbInstance::drop_index(
        const std::string&, const std::string&, const std::string&, txn::Transaction&
    ) { unsupported(); }

    void
    DetachedDbInstance::drop_table(
        const std::string&, const std::string&, txn::Transaction&
    ) { unsupported(); }

    void
    DetachedDbInstance::add_column(
        const std::string&, const std::string&,
        const ColumnDefinition&, txn::Transaction&
    ) { unsupported(); }

    UUID
    DetachedDbInstance::create_sequence(
        const std::string&,
        const std::string&,
        txn::Transaction&
    ) { unsupported(); }

    std::vector<MetaTable*>
    DetachedDbInstance::get_all_tables() { unsupported(); }

    std::vector<MetaSchema*>
    DetachedDbInstance::get_all_schemas() { unsupported(); }

} // namespace storage
