//
// Created by poproshaikin on 20.11.25.
//

#include "node_executor.hpp"

#include "../misc/include/convert.hpp"
#include "../storage/include/std_db_instance.hpp"

#include <algorithm>
#include <ranges>

namespace exq
{
    using namespace types;

    SeqScanNodeExecutor::SeqScanNodeExecutor(
        storage::IDbInstance& storage, const std::string& table_name, const std::string& schema_name
    )
        : table_name_(table_name), schema_name_(schema_name), db_(storage)
    {
    }

    void
    SeqScanNodeExecutor::open()
    {
        table_ = db_.seq_scan(table_name_, schema_name_);
    }

    bool
    SeqScanNodeExecutor::next(DataRow& out)
    {
        if (index_ >= table_.rows.size())
            return false;

        out = table_.rows[index_];
        index_++;
        return true;
    }

    void
    SeqScanNodeExecutor::close()
    {
    }

    OutputSchema
    SeqScanNodeExecutor::output_schema()
    {
        const auto* mt = db_.get_table(table_name_, schema_name_);

        OutputSchema output_schema;
        output_schema.reserve(mt->columns.size());

        for (const auto& column : mt->columns)
            output_schema.push_back({.name = column.name, .type = column.type});

        return output_schema;
    }

    FilterNodeExecutor::FilterNodeExecutor(
        const MetaTable& table, BinaryExpr&& condition, std::unique_ptr<INodeExecutor> child
    )
        : condition_(std::move(condition)), evaluator_(table), table_(table),
          child_(std::move(child))
    {
    }

    void
    FilterNodeExecutor::open()
    {
        child_->open();
    }

    bool
    FilterNodeExecutor::next(DataRow& out)
    {
        bool found = false;
        do
        {
            DataRow row;
            if (!child_->next(row))
                break;

            if (!evaluator_.evaluate(table_, row, condition_))
                continue;

            out = std::move(row);
            found = true;
        } while (!found);

        return found;
    }

    void
    FilterNodeExecutor::close()
    {
        child_->close();
    }

    OutputSchema
    FilterNodeExecutor::output_schema()
    {
        return child_->output_schema();
    }

    ProjectionNodeExecutor::ProjectionNodeExecutor(
        const MetaTable& table,
        const std::vector<std::string>& columns,
        std::unique_ptr<INodeExecutor> child
    )
        : table_(table), columns_(std::move(columns)), child_(std::move(child))
    {
    }

    void
    ProjectionNodeExecutor::open()
    {
        child_->open();
        std::vector<int64_t> indices;
        indices.reserve(columns_.size());

        for (const auto& column : columns_)
        {
            int64_t idx = table_.get_column_idx(column);
            assert(idx >= 0);
            indices.push_back(idx);
        }

        std::ranges::sort(indices);
        indices_ = std::move(indices);
    }

    bool
    ProjectionNodeExecutor::next(DataRow& out)
    {
        DataRow src_row;
        if (!child_->next(src_row))
            return false;

        for (auto idx : indices_)
        {
            out.tokens.push_back(src_row.tokens[idx]);
        }

        return true;
    }

    void
    ProjectionNodeExecutor::close()
    {
        child_->close();
    }

    OutputSchema
    ProjectionNodeExecutor::output_schema()
    {
        OutputSchema output_schema;
        output_schema.reserve(indices_.size());

        for (const auto& col_idx : indices_)
        {
            const auto& col = table_.columns[col_idx];
            output_schema.push_back({.name = col.name, .type = col.type});
        }

        return output_schema;
    }

    LimitNodeExecutor::LimitNodeExecutor(uint64_t limit, std::unique_ptr<INodeExecutor> child)
        : limit_(limit), child_(std::move(child))
    {
    }

    void
    LimitNodeExecutor::open()
    {
        child_->open();
    }

    bool
    LimitNodeExecutor::next(DataRow& out)
    {
        if (current_++ >= limit_)
            return false;

        return child_->next(out);
    }

    void
    LimitNodeExecutor::close()
    {
        child_->close();
    }

    OutputSchema
    LimitNodeExecutor::output_schema()
    {
        return child_->output_schema();
    }

    InsertNodeExecutor::InsertNodeExecutor(
        const std::string& table_name,
        const std::string& schema_name,
        storage::IDbInstance& storage,
        std::unique_ptr<INodeExecutor> child
    )
        : table_name_(table_name), schema_name_(schema_name), db_(storage),
          child_(std::move(child)), executed_(false)
    {
    }

    void
    InsertNodeExecutor::open()
    {
        child_->open();
    }

    bool
    InsertNodeExecutor::next(DataRow& out)
    {
        if (executed_)
            return false;

        int inserted_count = 0;

        auto txn = db_.make_txn();
        txn.begin();

        while (true)
        {
            DataRow row;
            if (!child_->next(row))
                break;

            db_.insert_row(table_name_, schema_name_, row.tokens, txn);
            inserted_count++;
        }

        txn.commit();

        DataToken affected_rows_count(misc::convert(inserted_count), DataType::INTEGER);
        out.tokens = {affected_rows_count};
        return false;
    }

    void
    InsertNodeExecutor::close()
    {
        child_->close();
    }

    OutputSchema
    InsertNodeExecutor::output_schema()
    {
        return {(OutputColumn){.name = "affected_rows", .type = DataType::INTEGER}};
    }

    ValuesNodeExecutor::ValuesNodeExecutor(const std::vector<DataRow>& rows) : rows_(rows)
    {
    }

    void
    ValuesNodeExecutor::open()
    {
    }

    bool
    ValuesNodeExecutor::next(DataRow& out)
    {
        if (idx_ >= rows_.size())
            return false;

        out = rows_[idx_++];
        return true;
    }

    void
    ValuesNodeExecutor::close()
    {
    }

    OutputSchema
    ValuesNodeExecutor::output_schema()
    {
        return OutputSchema{};
    }

    UpdateNodeExecutor::UpdateNodeExecutor(
        const std::string& table_name,
        const std::string& schema_name,
        storage::IDbInstance& db,
        const std::vector<Assignment>& asg,
        std::unique_ptr<INodeExecutor> child
    )
        : table_name_(table_name), schema_name_(schema_name), db_(db), assignments_(asg),
          child_(std::move(child)), executed_(false)
    {
    }

    void
    UpdateNodeExecutor::open()
    {
        child_->open();
    }

    bool
    UpdateNodeExecutor::next(DataRow& out)
    {
        if (executed_)
            return false;

        int updated_count = 0;
        std::vector<DataRow> rows;

        auto txn = db_.make_txn();
        txn.begin();

        while (true)
        {
            DataRow row;
            if (!child_->next(row))
                break;

            rows.push_back(std::move(row));
            updated_count++;
        }

        db_.update_row(table_name_, schema_name_, assignments_, rows, txn);
        executed_ = true;

        txn.commit();

        DataToken affected_rows_count(misc::convert(updated_count), DataType::INTEGER);
        out.tokens = {affected_rows_count};
        return false;
    }

    void
    UpdateNodeExecutor::close()
    {
        child_->close();
    }

    OutputSchema
    UpdateNodeExecutor::output_schema()
    {
        return {(OutputColumn){"affected rows", DataType::INTEGER}};
    }

    DeleteNodeExecutor::DeleteNodeExecutor(
        const std::string& table_name,
        const std::string& schema_name,
        storage::IDbInstance& db,
        std::unique_ptr<INodeExecutor> child
    )
        : table_name_(table_name), schema_name_(schema_name), db_(db), child_(std::move(child)),
          executed_(false)
    {
    }

    void
    DeleteNodeExecutor::open()
    {
        child_->open();
    }

    bool
    DeleteNodeExecutor::next(DataRow& out)
    {
        if (executed_)
            return false;

        int deleted_count = 0;
        std::vector<DataRow> rows;

        auto txn = db_.make_txn();
        txn.begin();

        while (true)
        {
            DataRow row;
            if (!child_->next(row))
                break;

            rows.push_back(std::move(row));
            deleted_count++;
        }

        db_.delete_rows(table_name_, schema_name_, rows, txn);
        executed_ = true;

        txn.commit();

        DataToken affected_rows_count(misc::convert(deleted_count), DataType::INTEGER);
        out.tokens = {affected_rows_count};
        return false;
    }

    void
    DeleteNodeExecutor::close()
    {
        child_->close();
    }

    OutputSchema
    DeleteNodeExecutor::output_schema()
    {
        return {(OutputColumn){"affected rows", DataType::INTEGER}};
    }

    CreateTableNodeExecutor::CreateTableNodeExecutor(
        const std::string& table_name,
        const MetaSchema& schema,
        const std::vector<ColumnDefinition>& columns,
        storage::IDbInstance& db
    )
        : table_name_(table_name), schema_(schema), columns_(columns), db_(db)
    {
    }

    void
    CreateTableNodeExecutor::open()
    {
    }

    bool
    CreateTableNodeExecutor::next(DataRow& out)
    {
        auto txn = db_.make_txn();
        txn.begin();
        db_.create_table(table_name_, schema_.name, columns_, txn);
        txn.commit();
        return false;
    }

    void
    CreateTableNodeExecutor::close()
    {
    }

    OutputSchema
    CreateTableNodeExecutor::output_schema()
    {
        return OutputSchema{};
    }

    CreateDbNodeExecutor::CreateDbNodeExecutor(const std::string& db_name) : db_name_(db_name)
    {
    }

    void
    CreateDbNodeExecutor::open()
    {
    }

    bool
    CreateDbNodeExecutor::next(DataRow& out)
    {
        auto config = Config::std(db_name_);
        auto db = storage::StdDbInstance(config);
        return false;
    }

    void
    CreateDbNodeExecutor::close()
    {
    }

    OutputSchema
    CreateDbNodeExecutor::output_schema()
    {
        return OutputSchema{};
    }

    CreateIndexNodeExecutor::CreateIndexNodeExecutor(
        const std::string& index_name,
        const std::string& table_name,
        const std::string& column_name,
        const std::string& schema_name,
        bool is_unique,
        storage::IDbInstance& db
    )
        : index_name_(index_name), column_name_(column_name), table_name_(table_name),
          schema_name_(schema_name), is_unique_(is_unique), db_(db)
    {
    }

    void
    CreateIndexNodeExecutor::open()
    {
    }

    bool
    CreateIndexNodeExecutor::next(DataRow& out)
    {
        auto txn = db_.make_txn();
        txn.begin();
        db_.create_index(index_name_, table_name_, column_name_, schema_name_, is_unique_, txn);
        txn.commit();
        return false;
    }

    void
    CreateIndexNodeExecutor::close()
    {
    }

    OutputSchema
    CreateIndexNodeExecutor::output_schema()
    {
        return OutputSchema{};
    }

    DropIndexNodeExecutor::DropIndexNodeExecutor(
        const std::string& index_name,
        const std::string& table_name,
        const std::string& schema_name,
        storage::IDbInstance& db
    )
        : index_name_(index_name), table_name_(table_name), schema_name_(schema_name), db_(db)
    {
    }

    void
    DropIndexNodeExecutor::open()
    {
    }

    bool
    DropIndexNodeExecutor::next(DataRow& out)
    {
        auto txn = db_.make_txn();
        txn.begin();
        db_.drop_index(index_name_, table_name_, schema_name_, txn);
        txn.commit();
        return false;
    }

    void
    DropIndexNodeExecutor::close()
    {
    }

    OutputSchema
    DropIndexNodeExecutor::output_schema()
    {
        return {};
    }

    std::unique_ptr<INodeExecutor>
    NodeExecutorFactory::from_plan(std::unique_ptr<IPlanNode>&& node, storage::IDbInstance& db)
    {
        switch (node->type())
        {
        case IPlanNode::Type::SEQ_SCAN:
        {
            const auto& seq_scan_node = static_cast<const SeqScanPlanNode&>(*node);
            SeqScanNodeExecutor executor(db, seq_scan_node.table_name, seq_scan_node.schema_name);

            return std::make_unique<SeqScanNodeExecutor>(std::move(executor));
        }
        case IPlanNode::Type::FILTER:
        {
            auto& filter_node = static_cast<FilterPlanNode&>(*node);
            FilterNodeExecutor executor(
                filter_node.table,
                std::move(filter_node.where),
                from_plan(std::move(filter_node.child), db)
            );

            return std::make_unique<FilterNodeExecutor>(std::move(executor));
        }
        case IPlanNode::Type::PROJECT:
        {
            auto& project_node = static_cast<ProjectPlanNode&>(*node);
            ProjectionNodeExecutor executor(
                project_node.table,
                project_node.columns,
                from_plan(std::move(project_node.child), db)
            );
            return std::make_unique<ProjectionNodeExecutor>(std::move(executor));
        }
        case IPlanNode::Type::LIMIT:
        {
            auto& limit_node = static_cast<LimitPlanNode&>(*node);
            LimitNodeExecutor executor(
                limit_node.limit, from_plan(std::move(limit_node.child), db)
            );
            return std::make_unique<LimitNodeExecutor>(std::move(executor));
        }
        case IPlanNode::Type::INSERT:
        {
            auto& insert_node = static_cast<InsertPlanNode&>(*node);
            InsertNodeExecutor executor(
                insert_node.table_name,
                insert_node.schema_name,
                db,
                from_plan(std::move(insert_node.child), db)
            );
            return std::make_unique<InsertNodeExecutor>(std::move(executor));
        }
        case IPlanNode::Type::VALUES:
        {
            auto& values_node = static_cast<ValuesPlanNode&>(*node);
            ValuesNodeExecutor executor(values_node.values);
            return std::make_unique<ValuesNodeExecutor>(std::move(executor));
        }
        case IPlanNode::Type::UPDATE:
        {
            auto& update_node = static_cast<UpdatePlanNode&>(*node);
            UpdateNodeExecutor executor(
                update_node.table_name,
                update_node.schema_name,
                db,
                update_node.assignments,
                from_plan(std::move(update_node.child), db)
            );
            return std::make_unique<UpdateNodeExecutor>(std::move(executor));
        }
        case IPlanNode::Type::DELETE:
        {
            auto& delete_node = static_cast<DeletePlanNode&>(*node);
            DeleteNodeExecutor executor(
                delete_node.table_name,
                delete_node.schema_name,
                db,
                from_plan(std::move(delete_node.child), db)
            );
            return std::make_unique<DeleteNodeExecutor>(std::move(executor));
        }
        case IPlanNode::Type::CREATE_TABLE:
        {
            auto& create_table_node = static_cast<CreateTablePlanNode&>(*node);
            CreateTableNodeExecutor executor(
                create_table_node.table_name,
                create_table_node.schema,
                create_table_node.columns,
                db
            );
            return std::make_unique<CreateTableNodeExecutor>(std::move(executor));
        }
        case IPlanNode::Type::CREATE_DB:
        {
            auto& create_db_node = static_cast<CreateDbPlanNode&>(*node);
            CreateDbNodeExecutor executor(create_db_node.db_name);
            return std::make_unique<CreateDbNodeExecutor>(std::move(executor));
        }
        case IPlanNode::Type::CREATE_INDEX:
        {
            auto& create_index_node = static_cast<CreateIndexPlanNode&>(*node);
            CreateIndexNodeExecutor executor(
                create_index_node.index_name,
                create_index_node.table_name,
                create_index_node.column_name,
                create_index_node.schema_name,
                create_index_node.is_unique,
                db
            );
            return std::make_unique<CreateIndexNodeExecutor>(std::move(executor));
        }
        case IPlanNode::Type::DROP_INDEX:
        {
            auto& drop_index_node = static_cast<DropIndexPlanNode&>(*node);
            DropIndexNodeExecutor executor(
                drop_index_node.index_name,
                drop_index_node.table_name,
                drop_index_node.schema_name,
                db
            );
            return std::make_unique<DropIndexNodeExecutor>(std::move(executor));
        }
        default:
            throw std::runtime_error(
                "NodeExecutorFactory::from_plan: failed to create executor tree for plan node of "
                "type " +
                std::to_string(static_cast<int>(node->type()))
            );
        }
    }

} // namespace exq