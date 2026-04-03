//
// Created by poproshaikin on 26.11.25.
//

#include "std_db_instance.hpp"

#include "BP_index_pager.hpp"
#include "exceptions.hpp"
#include "index_bplus_tree.hpp"
#include "io_manager_factory.hpp"
#include "logger.hpp"
#include "std_binary_serializer.hpp"
#include "utils.hpp"

#include "../misc/include/convert.hpp"
#include "../types/include/config.hpp"
#include "../wal/include/wal_manager_factory.hpp"

#include <unordered_set>

namespace storage
{
    using namespace types;
    using namespace misc;

    StdDbInstance::StdDbInstance(const Config& cfg) : cfg_(cfg)
    {
        if (!std::filesystem::exists(cfg.db_path))
            std::filesystem::create_directories(cfg.db_path);

        IOManagerFactory io_factory;
        io_manager_ = io_factory.make(cfg);
        wal::WalManagerFactory wal_factory;
        wal_manager_ = wal_factory.make(cfg);
        buffer_pool_ = std::make_unique<BufferPool>(*io_manager_);
        catalog_ = std::make_unique<CatalogCache>(*io_manager_);
        txn_manager_ = std::make_unique<txn::TransactionManager>(*wal_manager_, *buffer_pool_);
        recovery_manager_ =
            std::make_unique<recovery::RecoveryManager>(cfg_, *wal_manager_, *io_manager_);

        init();
    }

    void
    StdDbInstance::init()
    {
        io_manager_->init();
        recovery_manager_->recover();
        catalog_->hydrate();
        buffer_pool_->initialize();
    }

    StdDbInstance::~StdDbInstance()
    {
        io_manager_->write_cfg(cfg_);
    }

    bool
    StdDbInstance::needs_stream(IPlanNode& plan_node)
    {
        // TODO
        return false;
    }

    DataTable
    StdDbInstance::seq_scan(const std::string& table_name, const std::string& schema_name)
    {
        const auto* ms = catalog_->get_schema(schema_name);
        const auto* mt = catalog_->get_table(table_name, ms->id);
        const auto pages = buffer_pool_->get_table_data(mt->id);

        DataTable dt;
        dt.output_schema = convert(*mt);

        uint64_t rows_count = 0;
        for (auto* page : pages)
            rows_count += page->rows.size();

        dt.rows.reserve(rows_count);

        for (const auto& page : pages)
        {
            for (const auto& row : page->rows)
            {
                if (has_flag(row.flags, DataRowFlags::OBSOLETE))
                    continue;

                dt.rows.push_back(row);
            }
        }

        return dt;
    }

    txn::Transaction
    StdDbInstance::make_txn()
    {
        return txn_manager_->make_transaction();
    }

    ssize_t
    StdDbInstance::has_available_page(const std::vector<const DataPage*>& vec, size_t size) const
    {
        if (vec.size() == 0)
            return -1;

        for (size_t i = 0; i < vec.size(); i++)
            if (vec[i]->size + size <= DataPage::MAX_SIZE)
                return i;

        return -1;
    }

    bool
    StdDbInstance::is_row_obsolete(const RowPtr& row_ptr) const
    {
        const auto* page = buffer_pool_->get_dp(row_ptr.first);
        if (!page)
            return false;

        for (const auto& row : page->rows)
        {
            if (row.id == row_ptr.second)
                return has_flag(row.flags, DataRowFlags::OBSOLETE);
        }

        return false;
    }

    void
    StdDbInstance::insert_row(
        const std::string& table_name,
        const std::string& schema_name,
        std::vector<DataToken> row,
        txn::Transaction& txn
    )
    {
        const auto* ms = catalog_->get_schema(schema_name);
        auto* mt = catalog_->get_table(table_name, ms->id);
        const auto mt_unchanged = *mt;

        DataRow data_row;
        data_row.id = mt->last_rid++;
        data_row.tokens = std::move(row);
        size_t row_size = io_manager_->estimate_size(data_row);

        auto* page = buffer_pool_->prepare_dp(row_size, *mt);

        if (mt->indexes.size() > 0)
            insert_row_into_indexes(*mt, data_row, page->id);

        page->rows.push_back(data_row);

        mt->total_rows++;
        mt->live_rows++;

        InsertRecord insert_record(mt->id, page->id, data_row);
        UpdateTableRecord update_table_record(mt_unchanged, *mt);
        txn.append_log(insert_record);
        txn.append_log(update_table_record);
        const LSN page_lsn = txn.get_last_lsn();

        page->last_lsn = page_lsn;

        buffer_pool_->dirty_dp(page->id);
    }

    void
    StdDbInstance::insert_row_into_indexes(
        const MetaTable& mt, const DataRow& row, const DataPageId& page_id
    )
    {
        for (auto& mi : mt.indexes)
        {
            const auto col_idx = mt.get_column_idx(mi.column_id);
            if (col_idx < 0)
                throw std::runtime_error("Index column not found in table schema");

            const auto& key = row.tokens[static_cast<size_t>(col_idx)];
            const RowPtr row_ptr{page_id, row.id};

            BPIndexPager pager(*buffer_pool_, mt.id, mi.id);
            IndexBPlusTree tree(pager);

            if (mi.is_unique)
            {
                auto existing = tree.find(key);
                if (existing.has_value() && !is_row_obsolete(existing.value()))
                    throw UniqueConstraintViolation(mi.name);
            }

            tree.insert(key, row_ptr);
        }
    }

    void
    StdDbInstance::update_row(
        const std::string& table_name,
        const std::string& schema_name,
        RowUpdate update,
        const std::vector<DataRow>& rows,
        txn::Transaction& txn
    )
    {
        auto* ms = catalog_->get_schema(schema_name);
        auto* mt = catalog_->get_table(table_name, ms->id);
        const auto unchanged_mt = *mt;
        auto pages = buffer_pool_->get_table_data(mt->id);

        std::unordered_set<RowId> ids;
        for (const auto& row : rows)
            ids.insert(row.id);

        for (DataPage* page : pages)
        {
            bool updated = false;
            LSN page_lsn = page->last_lsn;

            for (auto& row : page->rows)
            {
                if (!ids.contains(row.id))
                    continue;

                if (has_flag(row.flags, DataRowFlags::OBSOLETE))
                    continue;

                DataRow new_row = row;
                new_row.id = ++mt->last_rid;

                row.flags |= DataRowFlags::OBSOLETE;

                for (const auto& assignment : update)
                {
                    ColumnId col_id = std::visit([](auto& a) { return a.first; }, assignment);
                    int64_t col_idx = mt->get_column_idx(col_id);
                    MetaColumn cola = mt->get_column(col_idx);

                    if (auto* lit = std::get_if<AssignLiteral>(&assignment))
                    {
                        new_row.tokens[col_idx] = lit->second;
                    }
                    else
                    {
                        auto* col = std::get_if<AssignColumn>(&assignment);
                        int src_idx = mt->get_column_idx(col->second);
                        new_row.tokens[col_idx] = row.tokens[src_idx];
                    }
                }

                mt->total_rows++;

                UpdateRecord update_record(mt->id, page->id, row, new_row);
                txn.append_log(update_record);
                page_lsn = std::max(page_lsn, txn.get_last_lsn());

                UpdateTableRecord update_table_record(unchanged_mt, *mt);
                txn.append_log(update_table_record);

                page->rows.push_back(new_row);
                page->max_rid = std::max(page->max_rid, new_row.id);

                if (mt->indexes.size() > 0)
                    insert_row_into_indexes(*mt, new_row, page->id);

                updated = true;
            }

            if (updated)
            {
                page->last_lsn = page_lsn;
                buffer_pool_->dirty_dp(page->id);
            }
        }
    }

    void
    StdDbInstance::delete_rows(
        const std::string& table_name,
        const std::string& schema_name,
        const std::vector<DataRow>& rows,
        txn::Transaction& txn
    )
    {
        auto* ms = catalog_->get_schema(schema_name);
        auto* mt = catalog_->get_table(table_name, ms->id);
        const auto unchanged_mt = *mt;
        auto pages = buffer_pool_->get_table_data(mt->id);

        std::unordered_set<RowId> ids;
        for (const auto& row : rows)
            ids.insert(row.id);

        for (auto& page : pages)
        {
            bool deleted = false;
            LSN page_lsn = page->last_lsn;

            for (auto& row : page->rows)
            {
                if (!ids.contains(row.id))
                    continue;

                if (has_flag(row.flags, DataRowFlags::OBSOLETE))
                    continue;

                deleted = true;

                row.flags |= DataRowFlags::OBSOLETE;

                mt->live_rows--;

                DeleteRecord record(mt->id, page->id, row);
                txn.append_log(record);
                page_lsn = std::max(page_lsn, txn.get_last_lsn());
                UpdateTableRecord update_table_record(unchanged_mt, *mt);
                txn.append_log(update_table_record);
            }

            if (deleted)
            {
                page->last_lsn = page_lsn;
                buffer_pool_->dirty_dp(page->id);
            }
        }
    }

    MetaTable*
    StdDbInstance::get_table(const std::string& table_name, const std::string& schema_name)
    {
        auto* ms = get_schema(schema_name);
        if (!ms) return nullptr;
        return catalog_->get_table(table_name, ms->id);
    }

    MetaTable*
    StdDbInstance::get_table(const TableIdentifier& identifier)
    {
        return get_table(
            identifier.table_name.value,
            identifier.schema_name.has_value() ? identifier.schema_name.value().value
                                               : cfg_.default_schema
        );
    }

    const Config&
    StdDbInstance::get_config() const
    {
        return cfg_;
    }

    MetaSchema*
    StdDbInstance::get_schema(const std::string& name)
    {
        return catalog_->get_schema(name);
    }

    bool
    StdDbInstance::exists_table(const std::string& table_name, const std::string& schema_name)
    {
        return get_table(table_name, schema_name) != nullptr;
    }

    bool
    StdDbInstance::exists_table(const TableIdentifier& identifier)
    {
        std::string schema_name = identifier.schema_name.has_value()
                                      ? identifier.schema_name.value().value
                                      : cfg_.default_schema;

        return exists_table(identifier.table_name.value, schema_name);
    }

    bool
    StdDbInstance::exists_db(const std::string& name)
    {
        return io_manager_->exists_db(name);
    }

    void
    StdDbInstance::create_table(
        const std::string& table_name,
        const std::string& schema_name,
        const std::vector<ColumnDefinition>& columns,
        txn::Transaction& txn
    )
    {
        auto* schema = catalog_->get_schema(schema_name);

        MetaTable mt;
        mt.id = Uuid::make();
        mt.name = table_name;
        mt.schema_id = schema->id;
        mt.last_rid = 0;
        mt.columns.reserve(columns.size());
        for (const auto& col_def : columns)
        {
            MetaColumn column(col_def);
            column.id = Uuid::make();
            column.table_id = mt.id;
            mt.columns.emplace_back(column);
        }

        CreateTableRecord record(mt);
        txn.append_log(record);

        catalog_->save_table(mt);
    }

    void
    StdDbInstance::create_schema(const std::string& schema_name, txn::Transaction& txn)
    {
        MetaSchema ms;
        ms.id = Uuid::make();
        ms.name = schema_name;
        ms.db_name = cfg_.db_name.value();

        CreateSchemaRecord record(ms);
        txn.append_log(record);

        catalog_->save_schema(ms);
    }

    bool
    StdDbInstance::exists_schema(const std::string& schema_name)
    {
        return catalog_->exists_schema(schema_name);
    }

    void
    StdDbInstance::create_index(
        const std::string& index_name,
        const std::string& table_name,
        const std::string& column_name,
        const std::string& schema_name,
        bool is_unique,
        txn::Transaction& txn
    )
    {
        const auto* schema = catalog_->get_schema(schema_name);
        auto* table = catalog_->get_table(table_name, schema->id);
        const auto& column = table->get_column(column_name);

        MetaIndex mi;
        mi.id = Uuid::make();
        mi.name = index_name;
        mi.column_id = column.id;
        mi.key_type = column.type;
        mi.is_unique = is_unique;
        mi.table_id = table->id;

        CreateIndexRecord record(mi);
        txn.append_log(record);

        buffer_pool_->create_table_index(schema_name, table_name, mi);

        const auto col_idx = table->get_column_idx(column.id);
        if (col_idx < 0)
            throw std::runtime_error("Index column not found in table schema");

        BPIndexPager pager(*buffer_pool_, table->id, mi.id);
        IndexBPlusTree tree(pager);

        auto pages = buffer_pool_->get_table_data(table->id);

        for (const auto& page : pages)
        {
            for (const auto& row : page->rows)
            {
                if (has_flag(row.flags, DataRowFlags::OBSOLETE))
                    continue;

                const auto& key = row.tokens[static_cast<size_t>(col_idx)];
                const RowPtr row_ptr{page->id, row.id};

                if (is_unique)
                {
                    auto existing = tree.find(key);
                    if (existing.has_value() && !is_row_obsolete(existing.value()))
                        throw UniqueConstraintViolation(mi.name);
                }

                tree.insert(key, row_ptr);
            }
        }

        table->indexes.push_back(std::move(mi));
    }

    bool
    StdDbInstance::exists_index(
        const std::string& index_name, const std::string& table_name, const std::string& schema_name
    )
    {
        return get_index(index_name, table_name, schema_name) != nullptr;
    }

    bool
    StdDbInstance::exists_index(
        const std::string& index_name, const TableIdentifier& table_identifier
    )
    {
        return get_index(index_name, table_identifier) != nullptr;
    }

    MetaIndex*
    StdDbInstance::get_index(
        const std::string& index_name, const std::string& table_name, const std::string& schema_name
    )
    {
        const auto* schema = catalog_->get_schema(schema_name);
        auto* table = catalog_->get_table(table_name, schema->id);

        for (auto& index : table->indexes)
            if (index.name == index_name)
                return &index;

        return nullptr;
    }

    MetaIndex*
    StdDbInstance::get_index(
        const std::string& index_name, const TableIdentifier& identifier
    )
    {
        std::string schema_name = identifier.schema_name.has_value()
                                      ? identifier.schema_name.value().value
                                      : cfg_.default_schema;

        return get_index(index_name, identifier.table_name.value, schema_name);
    }

    void
    StdDbInstance::drop_index(
        const std::string& index_name,
        const std::string& table_name,
        const std::string& schema_name,
        txn::Transaction& txn
    )
    {
        auto* table = get_table(table_name, schema_name);
        auto* index = get_index(index_name, table_name, schema_name);
        if (!index)
            throw std::runtime_error("StdDbInstance::drop_index");

        const auto index_unchanged = *index;

        std::erase_if(table->indexes, [&index](MetaIndex& index_entry) { return index_entry.id == index->id; });

        DropIndexRecord record(index_unchanged);
        txn.append_log(record);
    }
} // namespace storage