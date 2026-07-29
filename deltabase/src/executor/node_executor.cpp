//
// Created by poproshaikin on 20.11.25.
//

#include "node_executor.hpp"

#include <cassert>

#include "../misc/include/convert.hpp"
#include "../storage/include/storage_service_provider.hpp"
#include "include/information_schema_provider.hpp"

#include <algorithm>
#include <complex>
#include <ranges>

namespace exq
{
    using namespace types;

    void
    SeqScanNodeExecutor::open()
    {
        auto& dql = service_provider_.dql();
        cursor_ = dql.seq_scan_begin(mt_);
    }

    bool
    SeqScanNodeExecutor::next(DataRow& out)
    {
        auto& dql = service_provider_.dql();
        return dql.seq_scan_next(cursor_, out);
    }

    void
    SeqScanNodeExecutor::close()
    {
    }

    OutputSchema
    SeqScanNodeExecutor::output_schema()
    {
        OutputSchema output_schema;
        output_schema.reserve(mt_.columns.size());
        for (const auto& column : mt_.columns)
            output_schema.push_back({.name = column.name, .type = column.type});
        return output_schema;
    }

    FilterNodeExecutor::FilterNodeExecutor(
        const MetaTable& table,
        const BinaryExpr& condition,
        std::unique_ptr<INodeExecutor> child
    )
        : condition_(condition), evaluator_(table), table_(table),
          child_(std::move(child))
    {
    }

    VirtualTableNodeExecutor::VirtualTableNodeExecutor(
        const std::string& table_name,
        const std::string& schema_name,
        storage::StorageServiceProvider& service_provider
    ) : table_name_(table_name), schema_name_(schema_name), service_provider_(service_provider), index_(0)
    {
    }

    void
    VirtualTableNodeExecutor::open()
    {
        InformationSchemaProvider prov(service_provider_);
        TableIdentifier tid(
            SqlToken(SqlTokenType::IDENTIFIER, table_name_, 0, 0),
            SqlToken(SqlTokenType::IDENTIFIER, schema_name_, 0, 0)
        );
        mt_ = prov.get_virtual_table(tid);
        data_ = prov.get_virtual_data(tid);
        index_ = 0;
    }

    bool
    VirtualTableNodeExecutor::next(DataRow& out)
    {
        if (index_ >= data_.rows.size())
            return false;

        out = data_.rows[index_++];
        return true;
    }

    void
    VirtualTableNodeExecutor::close()
    {
    }

    OutputSchema
    VirtualTableNodeExecutor::output_schema()
    {
        OutputSchema schema;
        schema.reserve(mt_.columns.size());

        for (const auto& col : mt_.columns)
            schema.push_back({.name = col.name, .type = col.type});

        return schema;
    }

    IndexScanNodeExecutor::IndexScanNodeExecutor(
        const MetaTable& mt,
        const IndexId& index_id,
        const BinaryExpr& condition,
        storage::StorageServiceProvider& service_provider
    )
        : index_id_(index_id), condition_(condition),
          service_provider_(service_provider), mt_(mt)
    {
    }

    void
    IndexScanNodeExecutor::open()
    {
        auto& dql = service_provider_.dql();
        data_ = dql.index_scan(mt_, index_id_, condition_);
    }

    bool
    IndexScanNodeExecutor::next(DataRow& out)
    {
        if (index_ >= data_.rows.size())
            return false;

        out = data_.rows[index_++];
        return true;
    }

    void
    IndexScanNodeExecutor::close()
    {
    }

    OutputSchema
    IndexScanNodeExecutor::output_schema()
    {
        OutputSchema output_schema;
        output_schema.reserve(mt_.columns.size());
        for (const auto& column : mt_.columns)
            output_schema.push_back({.name = column.name, .type = column.type});
        return output_schema;
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
        const MetaTable& mt,
        storage::StorageServiceProvider& service_provider,
        const std::optional<std::vector<std::string> >& col_names,
        ExecutionContext& ctx,
        std::unique_ptr<INodeExecutor> child
    )
        : mt_(mt), service_provider_(service_provider),
          col_names_(col_names), child_(std::move(child)), ctx_(ctx)
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

        auto& dml = service_provider_.dml();
        auto& row_preprocessor = service_provider_.preprocessor();
        auto& constraint_enforcer = service_provider_.enforcer();

        int inserted_count = 0;

        while (true)
        {
            DataRow row;
            if (!child_->next(row))
                break;

            auto normalized_row = row.tokens;
            row_preprocessor.prepare_row(mt_, col_names_, normalized_row, *ctx_.txn);

            constraint_enforcer.validate_or_throw(mt_, normalized_row);

            dml.insert_row(mt_, normalized_row, *ctx_.txn);
            inserted_count++;
        }

        executed_ = true;

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
        const MetaTable& mt,
        storage::StorageServiceProvider& service_provider,
        const std::vector<Assignment>& asg,
        ExecutionContext& ctx,
        std::unique_ptr<INodeExecutor> child
    )
        : mt_(mt), service_provider_(service_provider), assignments_(asg),
          ctx_(ctx), child_(std::move(child)), executed_(false)
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

        auto& dml = service_provider_.dml();

        int updated_count = 0;
        std::vector<DataRow> rows;

        while (true)
        {
            DataRow row;
            if (!child_->next(row))
                break;

            rows.push_back(std::move(row));
            updated_count++;
        }

        dml.update_selected(mt_, assignments_, rows, *ctx_.txn);
        executed_ = true;

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
        const MetaTable& mt,
        storage::StorageServiceProvider& service_provider,
        ExecutionContext& ctx,
        std::unique_ptr<INodeExecutor> child
    )
        : mt_(mt), service_provider_(service_provider), ctx_(ctx),
          child_(std::move(child)), executed_(false)
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

        auto& dml = service_provider_.dml();

        int deleted_count = 0;
        std::vector<DataRow> rows;

        while (true)
        {
            DataRow row;
            if (!child_->next(row))
                break;

            rows.push_back(std::move(row));
            deleted_count++;
        }

        dml.delete_selected(mt_, rows, *ctx_.txn);
        executed_ = true;

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
        storage::StorageServiceProvider& service_provider,
        ExecutionContext& ctx
    )
        : table_name_(table_name), schema_(schema), columns_(columns),
          service_provider_(service_provider), ctx_(ctx)
    {
    }

    void
    CreateTableNodeExecutor::open()
    {
    }

    bool
    CreateTableNodeExecutor::next(DataRow& out)
    {
        service_provider_.ddl().create_table(table_name_, schema_.name, columns_, *ctx_.txn);
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

    AlterTableNodeExecutor::AlterTableNodeExecutor(
        const std::string& table_name,
        const MetaSchema& schema,
        const std::vector<AlterTableOperation>& operations,
        storage::StorageServiceProvider& service_provider,
        ExecutionContext& ctx
    ) : table_name_(table_name), schema_(schema), operations_(operations),
        service_provider_(service_provider), ctx_(ctx)
    {
    }

    void
    AlterTableNodeExecutor::open()
    {
    }

    bool
    AlterTableNodeExecutor::next(DataRow& out)
    {
        if (executed_)
            return false;

        for (const auto& operation : operations_)
        {
            if (auto* add_col = std::get_if<AddColumnOperation>(&operation))
            {
                service_provider_.ddl().add_column(table_name_, schema_.name, add_col->column, *ctx_.txn);
                executed_ = true;
            }
        }

        return false;
    }

    void
    AlterTableNodeExecutor::close()
    {
    }

    OutputSchema
    AlterTableNodeExecutor::output_schema()
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
        storage::StorageServiceProvider ssp(config);
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
        storage::StorageServiceProvider& service_provider,
        ExecutionContext& ctx
    )
        : index_name_(index_name), column_name_(column_name), table_name_(table_name),
          schema_name_(schema_name), is_unique_(is_unique),
          service_provider_(service_provider), ctx_(ctx)
    {
    }

    void
    CreateIndexNodeExecutor::open()
    {
    }

    bool
    CreateIndexNodeExecutor::next(DataRow& out)
    {
        service_provider_.ddl().create_index(
            index_name_,
            table_name_,
            column_name_,
            schema_name_,
            is_unique_,
            *ctx_.txn);
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
        storage::StorageServiceProvider& service_provider,
        ExecutionContext& ctx
    )
        : index_name_(index_name), table_name_(table_name), schema_name_(schema_name),
          service_provider_(service_provider), ctx_(ctx)
    {
    }

    void
    DropIndexNodeExecutor::open()
    {
    }

    bool
    DropIndexNodeExecutor::next(DataRow& out)
    {
        service_provider_.ddl().drop_index(index_name_, table_name_, schema_name_, *ctx_.txn);
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

    DropTableNodeExecutor::DropTableNodeExecutor(
        const std::string& table_name,
        const std::string& schema_name,
        storage::StorageServiceProvider& service_provider,
        ExecutionContext& ctx
    )
        : table_name_(table_name), schema_name_(schema_name),
          service_provider_(service_provider), ctx_(ctx)
    {
    }

    void
    DropTableNodeExecutor::open()
    {
    }

    bool
    DropTableNodeExecutor::next(DataRow& out)
    {
        service_provider_.ddl().drop_table(table_name_, schema_name_, *ctx_.txn);
        return false;
    }

    void
    DropTableNodeExecutor::close()
    {
    }

    OutputSchema
    DropTableNodeExecutor::output_schema()
    {
        return {};
    }

    std::unique_ptr<INodeExecutor>
    NodeExecutorFactory::from_plan(
        const IPlanNode& node,
        storage::StorageServiceProvider& ssp,
        ExecutionContext& ctx)
    {
        switch (node.type())
        {
        case IPlanNode::Type::SEQ_SCAN:
        {
            const auto& n = static_cast<const SeqScanPlanNode&>(node);
            const MetaTable& mt = *ssp.ddl().get_table(n.table_name, n.schema_name);
            return std::make_unique<SeqScanNodeExecutor>(ssp, mt);
        }
        case IPlanNode::Type::INDEX_SCAN:
        {
            const auto& n = static_cast<const IndexScanPlanNode&>(node);
            const MetaTable& mt = *ssp.ddl().get_table(n.table_name, n.schema_name);
            return std::make_unique<IndexScanNodeExecutor>(mt, n.index_id, n.condition, ssp);
        }
        case IPlanNode::Type::VIRTUAL_TABLE:
        {
            const auto& n = static_cast<const VirtualTablePlanNode&>(node);
            return std::make_unique<VirtualTableNodeExecutor>(n.table_name, n.schema_name, ssp);
        }
        case IPlanNode::Type::FILTER:
        {
            const auto& n = static_cast<const FilterPlanNode&>(node);
            return std::make_unique<FilterNodeExecutor>(
                n.table, n.where, from_plan(*n.child, ssp, ctx));
        }
        case IPlanNode::Type::PROJECT:
        {
            const auto& n = static_cast<const ProjectPlanNode&>(node);
            return std::make_unique<ProjectionNodeExecutor>(
                n.table, n.columns, from_plan(*n.child, ssp, ctx));
        }
        case IPlanNode::Type::LIMIT:
        {
            const auto& n = static_cast<const LimitPlanNode&>(node);
            return std::make_unique<LimitNodeExecutor>(n.limit, from_plan(*n.child, ssp, ctx));
        }
        case IPlanNode::Type::INSERT:
        {
            const auto& n = static_cast<const InsertPlanNode&>(node);
            const MetaTable& mt = *ssp.ddl().get_table(n.table_name, n.schema_name);
            return std::make_unique<InsertNodeExecutor>(
                mt, ssp, n.column_names, ctx, from_plan(*n.child, ssp, ctx));
        }
        case IPlanNode::Type::VALUES:
        {
            const auto& n = static_cast<const ValuesPlanNode&>(node);
            return std::make_unique<ValuesNodeExecutor>(n.values);
        }
        case IPlanNode::Type::UPDATE:
        {
            const auto& n = static_cast<const UpdatePlanNode&>(node);
            const MetaTable& mt = *ssp.ddl().get_table(n.table_name, n.schema_name);
            return std::make_unique<UpdateNodeExecutor>(
                mt, ssp, n.assignments, ctx, from_plan(*n.child, ssp, ctx));
        }
        case IPlanNode::Type::DELETE:
        {
            const auto& n = static_cast<const DeletePlanNode&>(node);
            const MetaTable& mt = *ssp.ddl().get_table(n.table_name, n.schema_name);
            return std::make_unique<DeleteNodeExecutor>(
                mt, ssp, ctx, from_plan(*n.child, ssp, ctx));
        }
        case IPlanNode::Type::CREATE_TABLE:
        {
            const auto& n = static_cast<const CreateTablePlanNode&>(node);
            return std::make_unique<CreateTableNodeExecutor>(n.table_name, n.schema, n.columns, ssp, ctx);
        }
        case IPlanNode::Type::ALTER_TABLE:
        {
            const auto& n = static_cast<const AlterTablePlanNode&>(node);
            return std::make_unique<AlterTableNodeExecutor>(n.table_name, n.schema, n.operations, ssp, ctx);
        }
        case IPlanNode::Type::CREATE_DB:
        {
            const auto& n = static_cast<const CreateDbPlanNode&>(node);
            return std::make_unique<CreateDbNodeExecutor>(n.db_name);
        }
        case IPlanNode::Type::CREATE_INDEX:
        {
            const auto& n = static_cast<const CreateIndexPlanNode&>(node);
            return std::make_unique<CreateIndexNodeExecutor>(
                n.index_name, n.table_name, n.column_name, n.schema_name, n.is_unique, ssp, ctx);
        }
        case IPlanNode::Type::DROP_INDEX:
        {
            const auto& n = static_cast<const DropIndexPlanNode&>(node);
            return std::make_unique<DropIndexNodeExecutor>(n.index_name, n.table_name, n.schema_name, ssp, ctx);
        }
        case IPlanNode::Type::DROP_TABLE:
        {
            const auto& n = static_cast<const DropTablePlanNode&>(node);
            return std::make_unique<DropTableNodeExecutor>(n.table_name, n.schema_name, ssp, ctx);
        }
        default:
            throw std::runtime_error(
                "NodeExecutorFactory::from_plan: failed to create executor tree for plan node of "
                "type " +
                std::to_string(static_cast<int>(node.type())));
        }
    }

} // namespace exq