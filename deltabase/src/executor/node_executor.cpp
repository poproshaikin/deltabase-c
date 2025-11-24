//
// Created by poproshaikin on 20.11.25.
//

#include "node_executor.hpp"

#include "convert.hpp"

#include <algorithm>

namespace exq
{
    using namespace types;

    SeqScanNodeExecutor::SeqScanNodeExecutor(
        storage::IStorage& storage,
        const std::string& table_name,
        const std::string& schema_name
    )
        : table_name_(table_name), schema_name_(schema_name), storage_(storage)
    {
    }

    void
    SeqScanNodeExecutor::open()
    {
        table_ = storage_.seq_scan(table_name_, schema_name_);
    }

    bool
    SeqScanNodeExecutor::next(DataRow& out)
    {
        if (index_++ < table_.rows.size())
            return false;

        out = table_.rows[index_];
        return true;
    }

    void
    SeqScanNodeExecutor::close()
    {
    }

    OutputSchema
    SeqScanNodeExecutor::output_schema()
    {
        MetaTable mt = storage_.get_table(table_name_, schema_name_);
        OutputSchema output_schema;
        output_schema.reserve(mt.columns.size());

        for (const auto& column : mt.columns)
            output_schema.push_back({.name = column.name, .type = column.type});

        return output_schema;
    }

    FilterNodeExecutor::FilterNodeExecutor(
        const MetaTable& table,
        BinaryExpr&& condition,
        std::unique_ptr<INodeExecutor> child
    )
        : condition_(std::move(condition)),
          evaluator_(table),
          table_(table),
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

            if (!evaluator_.evaluate(row, condition_))
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
        std::unique_ptr<INodeExecutor> child)
        : table_(table),
          columns_(std::move(columns)),
          child_(std::move(child))
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

    LimitNodeExecutor::LimitNodeExecutor(
        uint64_t limit,
        std::unique_ptr<INodeExecutor> child)
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
        storage::IStorage& storage,
        std::unique_ptr<INodeExecutor> child)
        : table_name_(table_name),
          schema_name_(schema_name),
          storage_(storage),
          child_(std::move(child))
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
        MetaTable table = storage_.get_table(table_name_, schema_name_);
        auto txn = storage_.begin_txn();

        while (true)
        {
            DataRow row;
            if (!child_->next(row))
                break;

            storage_.insert_row(table, row, txn);
            inserted_count++;
        }
        storage_.commit_txn(txn);

        DataToken affected_rows_count(misc::convert(inserted_count), DataType::INTEGER);
        out.tokens = { affected_rows_count };
        return true;
    }

    void
    InsertNodeExecutor::close()
    {
        child_->close();
    }

    OutputSchema
    InsertNodeExecutor::output_schema()
    {
        return { (OutputColumn){ .name = "affected_rows", .type = DataType::INTEGER } };
    }

    NodeExecutorFactory::NodeExecutorFactory(storage::IStorage& storage) : storage_(storage)
    {
    }

    std::unique_ptr<INodeExecutor>
    NodeExecutorFactory::from_plan(std::unique_ptr<IPlanNode>&& node)
    {
        switch (node->type())
        {
        case IPlanNode::Type::SEQ_SCAN:
        {
            const auto& seq_scan_node = static_cast<const SeqScanPlanNode&>(*node);
            SeqScanNodeExecutor executor(
                storage_,
                seq_scan_node.table_name,
                seq_scan_node.schema_name
            );

            return std::make_unique<SeqScanNodeExecutor>(std::move(executor));
        }
        case IPlanNode::Type::FILTER:
        {
            auto& filter_node = static_cast<FilterPlanNode&>(*node);
            FilterNodeExecutor executor(
                filter_node.table,
                std::move(filter_node.where),
                from_plan(std::move(filter_node.child))
            );

            return std::make_unique<FilterNodeExecutor>(std::move(executor));
        }
        case IPlanNode::Type::PROJECT:
        {
            auto& project_node = static_cast<ProjectPlanNode&>(*node);
            ProjectionNodeExecutor executor(
                project_node.table,
                project_node.columns,
                from_plan(std::move(project_node.child))
            );
            return std::make_unique<ProjectionNodeExecutor>(std::move(executor));
        }
        case IPlanNode::Type::LIMIT:
        {
            auto& limit_node = static_cast<LimitPlanNode&>(*node);
            LimitNodeExecutor executor(limit_node.limit, from_plan(std::move(limit_node.child)));
            return std::make_unique<LimitNodeExecutor>(std::move(executor));
        }
        case IPlanNode::Type::INSERT:
        {
            auto& insert_node = static_cast<InsertPlanNode&>(*node);
            InsertNodeExecutor
        }
        default:
            throw std::runtime_error("NodeExecutorFactory::from_plan");
        }
    }

}