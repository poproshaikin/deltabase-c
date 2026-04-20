//
// Created by poproshaikin on 26.11.25.
//

#include "std_db_instance.hpp"

#include "BP_index_pager.hpp"
#include "exceptions.hpp"
#include "index_bplus_tree.hpp"
#include "io_manager_factory.hpp"
#include "logger.hpp"
#include "std_storage_serializer.hpp"
#include "utils.hpp"

#include "../misc/include/convert.hpp"
#include "../types/include/config.hpp"
#include "../wal/include/wal_manager_factory.hpp"

#include <unordered_set>

namespace storage
{
    using namespace types;
    using namespace misc;
    using InstanceGuard = std::lock_guard<std::recursive_mutex>;

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
        InstanceGuard guard(mtx_);
        io_manager_->init();
        recovery_manager_->recover();
        catalog_->hydrate();
        buffer_pool_->initialize();
    }

    StdDbInstance::~StdDbInstance()
    {
        InstanceGuard guard(mtx_);
        io_manager_->write_cfg(cfg_);
    }

    DataTable
    StdDbInstance::seq_scan(const std::string& table_name, const std::string& schema_name)
    {
        InstanceGuard guard(mtx_);
        const auto* ms = catalog_->get_schema(schema_name);
        const auto* mt = catalog_->get_table(table_name, ms->id);

        DataTable dt;
        dt.output_schema = convert(*mt);
        dt.rows.reserve(mt->total_rows);

        auto cursor = seq_scan_begin(table_name, schema_name);
        DataRow row;
        while (seq_scan_next(cursor, row))
            dt.rows.push_back(row);

        return dt;
    }

    ScanCursor
    StdDbInstance::seq_scan_begin(const std::string& table_name, const std::string& schema_name)
    {
        InstanceGuard guard(mtx_);
        const auto* ms = catalog_->get_schema(schema_name);
        auto* mt = catalog_->get_table(table_name, ms->id);
        const auto pages = buffer_pool_->get_table_data(mt->id);

        ScanCursor cursor{};
        cursor.page = DataPageId::null();
        cursor.slot = 0;
        cursor.chunk_size = 0;
        cursor.initialized = true;

        if (pages.empty())
            return cursor;

        std::unordered_set<DataPageId> referenced_pages;
        referenced_pages.reserve(pages.size());

        for (const auto* page : pages)
        {
            if (!page)
                continue;

            if (page->next != DataPageId::null())
                referenced_pages.insert(page->next);
        }

        for (const auto* page : pages)
        {
            if (!page)
                continue;

            if (!referenced_pages.contains(page->id))
            {
                cursor.page = page->id;
                return cursor;
            }
        }

        for (const auto* page : pages)
        {
            if (!page)
                continue;

            cursor.page = page->id;
            return cursor;
        }

        return cursor;
    }

    bool
    StdDbInstance::seq_scan_next(ScanCursor& cursor, DataRow& out)
    {
        InstanceGuard guard(mtx_);
        if (!cursor.initialized)
            return false;

        while (cursor.page != DataPageId::null())
        {
            auto* page = buffer_pool_->get_dp(cursor.page);
            if (!page)
            {
                cursor.page = DataPageId::null();
                return false;
            }

            while (cursor.slot < static_cast<int>(page->rows.size()))
            {
                const auto& row = page->rows[static_cast<size_t>(cursor.slot++)];
                if (has_flag(row.flags, DataRowFlags::OBSOLETE))
                    continue;

                out = row;
                return true;
            }

            cursor.page = page->next;
            cursor.slot = 0;
        }

        return false;
    }

    DataTable
    StdDbInstance::index_scan(
        const std::string& table_name,
        const std::string& schema_name,
        const IndexId& index_id,
        const BinaryExpr& condition
    )
    {
        InstanceGuard guard(mtx_);
        const auto* ms = catalog_->get_schema(schema_name);
        const auto* mt = catalog_->get_table(table_name, ms->id);

        const MetaIndex* meta_index = nullptr;
        for (const auto& index : mt->indexes)
        {
            if (index.id == index_id)
            {
                meta_index = &index;
                break;
            }
        }

        if (!meta_index)
            throw std::runtime_error("StdDbInstance::index_scan: index is not part of table");

        DataTable dt;
        dt.output_schema = convert(*mt);

        auto value_of_node = [&](const AstNode& node, const DataRow& row) -> DataToken
        {
            if (node.type == AstNodeType::IDENTIFIER)
            {
                const auto& col = std::get<SqlToken>(node.value);
                const int64_t col_idx = mt->get_column_idx(col.value);
                if (col_idx < 0)
                    throw std::runtime_error("StdDbInstance::index_scan: column not found");

                return row.tokens[static_cast<size_t>(col_idx)];
            }

            if (node.type == AstNodeType::LITERAL)
                return DataToken(std::get<SqlToken>(node.value));

            throw std::runtime_error("StdDbInstance::index_scan: unsupported condition node");
        };

        auto matches_condition = [&](const DataRow& row) -> bool
        {
            const auto left = value_of_node(*condition.left, row);
            const auto right = value_of_node(*condition.right, row);

            switch (condition.op)
            {
            case AstOperator::EQ:
                return left == right;
            case AstOperator::NEQ:
                return left != right;
            case AstOperator::LT:
                return left < right;
            case AstOperator::LTE:
                return left <= right;
            case AstOperator::GR:
                return left > right;
            case AstOperator::GRE:
                return left >= right;
            default:
                throw std::runtime_error("StdDbInstance::index_scan: unsupported condition op");
            }
        };

        auto append_row_by_ptr = [&](const RowPtr& row_ptr)
        {
            const auto* page = buffer_pool_->get_dp(row_ptr.first);
            if (!page)
                return;

            for (const auto& row : page->rows)
            {
                if (row.id != row_ptr.second)
                    continue;

                if (has_flag(row.flags, DataRowFlags::OBSOLETE))
                    return;

                if (matches_condition(row))
                    dt.rows.push_back(row);

                return;
            }
        };

        auto is_eq_for_indexed_column = [&](const AstNode* id_node, const AstNode* lit_node) -> bool
        {
            if (id_node->type != AstNodeType::COLUMN_IDENTIFIER ||
                lit_node->type != AstNodeType::LITERAL)
                return false;

            const auto& column_token = std::get<SqlToken>(id_node->value);
            const int64_t col_idx = mt->get_column_idx(column_token.value);
            if (col_idx < 0)
                return false;

            return mt->columns[static_cast<size_t>(col_idx)].id == meta_index->column_id;
        };

        BPIndexPager pager(*buffer_pool_, mt->id, index_id);
        IndexBPlusTree tree(pager);

        if (condition.op == AstOperator::EQ)
        {
            if (is_eq_for_indexed_column(condition.left.get(), condition.right.get()))
            {
                const auto key = DataToken(std::get<SqlToken>(condition.right->value));
                auto row_ptr = tree.find(key);
                if (row_ptr.has_value())
                    append_row_by_ptr(row_ptr.value());
                return dt;
            }

            if (is_eq_for_indexed_column(condition.right.get(), condition.left.get()))
            {
                const auto key = DataToken(std::get<SqlToken>(condition.left->value));
                auto row_ptr = tree.find(key);
                if (row_ptr.has_value())
                    append_row_by_ptr(row_ptr.value());
                return dt;
            }
        }

        auto* page = pager.get_page(pager.root_page_id());
        if (!page)
            return dt;

        while (!page->is_leaf)
        {
            const auto& internal = std::get<InternalIndexNode>(page->data);
            if (internal.children.empty())
                return dt;

            page = pager.get_page(internal.children.front());
            if (!page)
                throw std::runtime_error("StdDbInstance::index_scan: broken index tree");
        }

        while (page)
        {
            const auto& leaf = std::get<LeafIndexNode>(page->data);
            for (const auto& row_ptr : leaf.rows)
                append_row_by_ptr(row_ptr);

            if (leaf.next_leaf == 0)
                break;

            page = pager.get_page(leaf.next_leaf);
            if (!page)
                throw std::runtime_error("StdDbInstance::index_scan: broken leaf chain");
        }

        return dt;
    }

    txn::Transaction
    StdDbInstance::make_txn()
    {
        InstanceGuard guard(mtx_);
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
        InstanceGuard guard(mtx_);
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
        const std::optional<std::vector<std::string>>& cols,
        std::vector<DataToken> row,
        txn::Transaction& txn
    )
    {
        InstanceGuard guard(mtx_);
        const auto* ms = catalog_->get_schema(schema_name);
        auto* mt = catalog_->get_table(table_name, ms->id);
        const auto mt_unchanged = *mt;

        auto new_row = mt->make_row(cols, row);
        size_t row_size = io_manager_->estimate_size(new_row);

        const auto pages_before = buffer_pool_->get_table_data(mt->id);

        auto* page = buffer_pool_->prepare_dp(row_size, *mt);

        bool is_new_page = true;
        for (const auto* existing_page : pages_before)
        {
            if (!existing_page)
                continue;

            if (existing_page->id == page->id)
            {
                is_new_page = false;
                break;
            }
        }

        if (is_new_page)
        {
            DataPage* tail_page = nullptr;

            for (auto* existing_page : pages_before)
            {
                if (!existing_page)
                    continue;

                if (existing_page->next != DataPageId::null())
                    continue;

                if (!tail_page || existing_page->max_rid > tail_page->max_rid)
                    tail_page = existing_page;
            }

            if (tail_page)
            {
                tail_page->next = page->id;
                buffer_pool_->dirty_dp(tail_page->id);
            }
        }

        std::vector<IndexId> touched_indexes;
        if (mt->indexes.size() > 0)
            touched_indexes = insert_row_into_indexes(*mt, new_row, page->id);

        page->rows.push_back(new_row);

        InsertRecord insert_record(mt->id, page->id, new_row);
        UpdateTableRecord update_table_record(mt_unchanged, *mt);
        txn.append_log(insert_record);
        txn.append_log(update_table_record);
        const LSN page_lsn = txn.get_last_lsn();

        page->last_lsn = page_lsn;
        for (const auto& index_id : touched_indexes)
            buffer_pool_->set_if_lsn(index_id, page_lsn);

        buffer_pool_->dirty_dp(page->id);
    }

    std::vector<IndexId>
    StdDbInstance::insert_row_into_indexes(
        const MetaTable& mt, const DataRow& row, const DataPageId& page_id
    )
    {
        InstanceGuard guard(mtx_);
        std::vector<IndexId> touched_indexes;
        touched_indexes.reserve(mt.indexes.size());

        for (auto& mi : mt.indexes)
        {
            const auto col_idx = mt.get_column_idx(mi.column_id);
            if (col_idx < 0)
                throw std::runtime_error("Index column not found in table schema");

            const auto& key = row.tokens[static_cast<size_t>(col_idx)];
            
            // NULL values are not indexed
            if (key.type == DataType::_NULL)
                continue;
            
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
            touched_indexes.push_back(mi.id);
        }

        return touched_indexes;
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
        InstanceGuard guard(mtx_);
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
                {
                    auto touched_indexes = insert_row_into_indexes(*mt, new_row, page->id);
                    for (const auto& index_id : touched_indexes)
                        buffer_pool_->set_if_lsn(index_id, page_lsn);
                }

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
        InstanceGuard guard(mtx_);
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
        InstanceGuard guard(mtx_);
        auto* ms = get_schema(schema_name);
        if (!ms) return nullptr;
        return catalog_->get_table(table_name, ms->id);
    }

    MetaTable*
    StdDbInstance::get_table(const TableIdentifier& identifier)
    {
        InstanceGuard guard(mtx_);
        return get_table(
            identifier.table_name.value,
            identifier.schema_name.has_value() ? identifier.schema_name.value().value
                                               : cfg_.default_schema
        );
    }

    const Config&
    StdDbInstance::get_config() const
    {
        InstanceGuard guard(mtx_);
        return cfg_;
    }

    MetaSchema*
    StdDbInstance::get_schema(const std::string& name)
    {
        InstanceGuard guard(mtx_);
        return catalog_->get_schema(name);
    }

    bool
    StdDbInstance::exists_table(const std::string& table_name, const std::string& schema_name)
    {
        InstanceGuard guard(mtx_);
        return get_table(table_name, schema_name) != nullptr;
    }

    bool
    StdDbInstance::exists_table(const TableIdentifier& identifier)
    {
        InstanceGuard guard(mtx_);
        std::string schema_name = identifier.schema_name.has_value()
                                      ? identifier.schema_name.value().value
                                      : cfg_.default_schema;

        return exists_table(identifier.table_name.value, schema_name);
    }

    bool
    StdDbInstance::exists_db(const std::string& name)
    {
        InstanceGuard guard(mtx_);
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
        InstanceGuard guard(mtx_);
        auto* schema = catalog_->get_schema(schema_name);

        MetaTable mt;
        mt.id = UUID::make();
        mt.name = table_name;
        mt.schema_id = schema->id;
        mt.last_rid = 0;
        mt.columns.reserve(columns.size());
        for (const auto& col_def : columns)
        {
            MetaColumn column(col_def);
            column.id = UUID::make();
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
        InstanceGuard guard(mtx_);
        MetaSchema ms;
        ms.id = UUID::make();
        ms.name = schema_name;
        ms.db_name = cfg_.db_name.value();

        CreateSchemaRecord record(ms);
        txn.append_log(record);

        catalog_->save_schema(ms);
    }

    bool
    StdDbInstance::exists_schema(const std::string& schema_name)
    {
        InstanceGuard guard(mtx_);
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
        InstanceGuard guard(mtx_);
        const auto* schema = catalog_->get_schema(schema_name);
        auto* table = catalog_->get_table(table_name, schema->id);
        const auto& column = table->get_column(column_name);

        MetaIndex mi;
        mi.id = UUID::make();
        mi.name = index_name;
        mi.column_id = column.id;
        mi.key_type = column.type;
        mi.is_unique = is_unique;
        mi.table_id = table->id;

        CreateIndexRecord record(mi);
        txn.append_log(record);

        buffer_pool_->create_table_index(schema_name, *table, mi, txn.get_last_lsn());

        const auto col_idx = table->get_column_idx(column.id);
        if (col_idx < 0)
            throw std::runtime_error("Index column not found in table schema");

        // Pre-validate unique constraint on existing data before creating index file
        if (is_unique)
        {
            auto pages = buffer_pool_->get_table_data(table->id);
            std::unordered_set<std::string> seen_values;

            for (const auto& page : pages)
            {
                for (const auto& row : page->rows)
                {
                    if (has_flag(row.flags, DataRowFlags::OBSOLETE))
                        continue;

                    const auto& key = row.tokens[static_cast<size_t>(col_idx)];
                    
                    // NULL values are not indexed and don't violate uniqueness
                    if (key.type == DataType::_NULL)
                        continue;
                    
                    std::string key_str(key.bytes.begin(), key.bytes.end());
                    
                    if (seen_values.count(key_str) > 0)
                    {
                        throw UniqueConstraintViolation(
                            "Cannot create unique index '" + index_name + "' on column '" + 
                            column_name + "': table '" + table_name + "' contains duplicate values"
                        );
                    }
                    
                    seen_values.insert(key_str);
                }
            }
        }

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
                
                // NULL values are not indexed
                if (key.type == DataType::_NULL)
                    continue;
                
                const RowPtr row_ptr{page->id, row.id};

                tree.insert(key, row_ptr);
            }
        }

        buffer_pool_->set_if_lsn(mi.id, txn.get_last_lsn());

        table->indexes.push_back(std::move(mi));
    }

    bool
    StdDbInstance::exists_index(
        const std::string& index_name, const std::string& table_name, const std::string& schema_name
    )
    {
        InstanceGuard guard(mtx_);
        return get_index(index_name, table_name, schema_name) != nullptr;
    }

    bool
    StdDbInstance::exists_index(
        const std::string& index_name, const TableIdentifier& table_identifier
    )
    {
        InstanceGuard guard(mtx_);
        return get_index(index_name, table_identifier) != nullptr;
    }

    MetaIndex*
    StdDbInstance::get_index(
        const std::string& index_name, const std::string& table_name, const std::string& schema_name
    )
    {
        InstanceGuard guard(mtx_);
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
        InstanceGuard guard(mtx_);
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
        InstanceGuard guard(mtx_);
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