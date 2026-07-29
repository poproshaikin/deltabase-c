//
// Created by poproshaikin on 09.11.25.
//

#include "include/std_planner.hpp"

#include <algorithm>
#include <cmath>
#include <format>

namespace exq
{
    using namespace types;

    namespace
    {
        constexpr double k_default_filter_selectivity = 0.3;
        constexpr double k_project_selectivity = 1.0;
        constexpr double k_seq_scan_stream_threshold_rows = 2048.0;
    }

    StdPlanner::StdPlanner(const Config& db_config, storage::StorageServiceProvider& ssp)
        : ssp_(ssp), db_config_(db_config), info_schema_provider_(ssp_)
    {
    }

    double
    StdPlanner::estimate_seq_scan_selectivity(const MetaTable& table, const IPlanNode& node)
    {
        switch (node.type())
        {
        case IPlanNode::Type::SEQ_SCAN:
        {
            if (table.total_rows == 0)
                return 0.0;

            return static_cast<double>(table.live_rows) / static_cast<double>(table.total_rows);
        }

        case IPlanNode::Type::FILTER:
        {
            const auto& filter_node = static_cast<const FilterPlanNode&>(node);
            return estimate_seq_scan_selectivity(table, *filter_node.child) *
                   k_default_filter_selectivity;
        }

        case IPlanNode::Type::PROJECT:
        {
            const auto& project_node = static_cast<const ProjectPlanNode&>(node);
            return estimate_seq_scan_selectivity(table, *project_node.child) *
                   k_project_selectivity;
        }

        case IPlanNode::Type::LIMIT:
        {
            const auto& limit_node = static_cast<const LimitPlanNode&>(node);
            if (table.live_rows == 0)
                return 0.0;

            const auto child_sel = estimate_seq_scan_selectivity(table, *limit_node.child);
            const auto estimated_rows = static_cast<double>(table.live_rows) * child_sel;
            const auto capped_rows =
                std::min(estimated_rows, static_cast<double>(limit_node.limit));
            return capped_rows / static_cast<double>(table.live_rows);
        }

        default:
            return 1.0;
        }
    }

    bool
    StdPlanner::should_stream_for_seq_scan(const MetaTable& table, const IPlanNode& node)
    {
        const auto sel = estimate_seq_scan_selectivity(table, node);
        const auto estimated_rows = static_cast<double>(table.total_rows) * sel;
        return estimated_rows >= k_seq_scan_stream_threshold_rows;
    }

    QueryPlan
    StdPlanner::plan(AstNode&& ast)
    {
        if (ast.type == AstNodeType::SELECT)
        {
            return plan(std::get<SelectStmt>(ast.value));
        }
        if (ast.type == AstNodeType::INSERT)
        {
            return plan(std::get<InsertStmt>(ast.value));
        }
        if (ast.type == AstNodeType::UPDATE)
        {
            return plan(std::get<UpdateStmt>(ast.value));
        }
        if (ast.type == AstNodeType::DELETE)
        {
            return plan(std::get<DeleteStmt>(ast.value));
        }
        if (ast.type == AstNodeType::CREATE_DATABASE)
        {
            return plan(std::get<CreateDatabaseStmt>(ast.value));
        }
        if (ast.type == AstNodeType::CREATE_TABLE)
        {
            return plan(std::get<CreateTableStmt>(ast.value));
        }
        if (ast.type == AstNodeType::CREATE_INDEX)
        {
            return plan(std::get<CreateIndexStmt>(ast.value));
        }
        if (ast.type == AstNodeType::DROP_INDEX)
        {
            return plan(std::get<DropIndexStmt>(ast.value));
        }
        if (ast.type == AstNodeType::DROP_TABLE)
        {
            return plan(std::get<DropTableStmt>(ast.value));
        }
        if (ast.type == AstNodeType::ALTER_TABLE)
        {
            return plan(std::get<AlterTableStmt>(ast.value));
        }

        throw std::runtime_error(
            std::format(
                "Unsupported query in StdPlanner::plan: {}",
                static_cast<std::underlying_type_t<decltype(ast.type)>>(ast.type)
            )
        );
    }

    double
    estimate_selectivity(const BinaryExpr& condition, const MetaIndex& idx)
    {
        switch (condition.op)
        {
        case AstOperator::EQ:
            return idx.is_unique ? 0.1 : 1.0;
        case AstOperator::LT:
        case AstOperator::LTE:
        case AstOperator::GR:
        case AstOperator::GRE:
            return 0.3;
        default:
            return 1.0;
        }
    }

    bool
    is_null_literal(const std::unique_ptr<AstNode>& node)
    {
        if (!node || node->type != AstNodeType::LITERAL)
            return false;

        const auto& token = std::get<SqlToken>(node->value);
        return std::holds_alternative<SqlLiteral>(token.detail) &&
               std::get<SqlLiteral>(token.detail) == SqlLiteral::_NULL;
    }

    bool
    is_null_predicate(const BinaryExpr& condition)
    {
        if (condition.op == AstOperator::IS)
            return true;

        if (condition.op != AstOperator::EQ && condition.op != AstOperator::NEQ)
            return false;

        return is_null_literal(condition.left) || is_null_literal(condition.right);
    }

    double
    estimate_index_scan(const MetaTable& table, const MetaIndex& idx, const BinaryExpr& condition)
    {
        double N = table.total_rows;
        double live = table.live_rows;

        if (live == 0)
            return 0;

        double live_ratio = live / N;

        double sel = estimate_selectivity(condition, idx);
        double K_live = live * sel;
        double K_index = K_live / live_ratio;
        double btree_cost = std::log2(N);

        return btree_cost + K_index;
    }

    IPlanNode::Type
    choose_scan_type(
        const MetaTable& table,
        const BinaryExpr* condition,
        const MetaIndex** chosen_index
    )
    {
        double best_cost = table.total_rows;
        auto best = IPlanNode::Type::SEQ_SCAN;

        if (!condition)
            return best;

        if (is_null_predicate(*condition))
            return best;

        for (const auto& idx : table.indexes)
        {
            // TODO
            if (condition->left->type != AstNodeType::IDENTIFIER)
                throw std::runtime_error("choose_scan_type: left token must be column identifier");

            auto& column = table.get_column(std::get<SqlToken>(condition->left->value).value);
            if (idx.column_id != column.id)
                continue;

            auto cost = estimate_index_scan(table, idx, *condition);
            if (cost < best_cost)
            {
                best_cost = cost;
                best = IPlanNode::Type::INDEX_SCAN;
                *chosen_index = &idx;
            }
        }

        return best;
    }

    QueryPlan
    StdPlanner::plan(SelectStmt& stmt) const
    {
        std::unique_ptr<IPlanNode> node;
        bool index = false;

        const std::string schema_name = stmt.table.schema_name.has_value()
                                                ? stmt.table.schema_name.value().value
                                                : db_config_.default_schema;

        IPlanNode::Type scan_type = IPlanNode::Type::SEQ_SCAN;
        const MetaTable* table = nullptr;
        std::optional<MetaTable> virtual_table_storage;

        if (info_schema_provider_.is_virtual(stmt.table))
        {
            // Virtual table: minimal plan node, no index optimization
            node = std::make_unique<VirtualTablePlanNode>(stmt.table.table_name, schema_name);
            virtual_table_storage = info_schema_provider_.get_virtual_table(stmt.table);
            table = &virtual_table_storage.value();
        }
        else
        {
            // Real table: full planning with index optimization
            table = ssp_.ddl().get_table(stmt.table);
            const auto* condition_ptr = stmt.where ? &(*stmt.where) : nullptr;

            const MetaIndex* chosen_index = nullptr;
            scan_type = choose_scan_type(*table, condition_ptr, &chosen_index);

            if (scan_type == IPlanNode::Type::INDEX_SCAN)
            {
                node = std::make_unique<IndexScanPlanNode>(
                    stmt.table.table_name,
                    schema_name,
                    chosen_index->id,
                    std::move(*stmt.where));

                index = true;
            }
            else
            {
                node = std::make_unique<SeqScanPlanNode>(stmt.table.table_name, schema_name);
            }
        }

        // 2. WHERE
        if (stmt.where && !index && table)
        {
            auto filter = std::make_unique<FilterPlanNode>(
                *table,
                std::move(*stmt.where),
                std::move(node));

            node = std::move(filter);
        }

        // 3. PROJECT
        if (!stmt.columns.empty() && table)
        {
            std::vector<std::string> cols;
            for (auto& c : stmt.columns)
                cols.push_back(c.value);

            auto project = std::make_unique<ProjectPlanNode>(
                *table,
                cols,
                std::move(node));

            node = std::move(project);
        }

        // 4. LIMIT
        if (stmt.limit)
        {
            auto limit = std::make_unique<LimitPlanNode>(stmt.limit.value(), std::move(node));
            node = std::move(limit);
        }

        QueryPlan plan;
        plan.type = QueryPlan::Type::SELECT;
        if (scan_type == IPlanNode::Type::SEQ_SCAN && table != nullptr && node->type() == IPlanNode::Type::SEQ_SCAN)
            plan.needs_stream = should_stream_for_seq_scan(*table, *node);
        else
            plan.needs_stream = false;

        plan.root = std::move(node);
        plan.db_specific = true;
        plan.needs_txn = false;
        return plan;
    }

    QueryPlan
    StdPlanner::plan(InsertStmt& stmt) const
    {
        std::vector<DataRow> rows;
        for (auto& [vals] : stmt.values)
            rows.emplace_back(vals);

        auto values_node = std::make_unique<ValuesPlanNode>(std::move(rows));

        std::optional<std::vector<std::string>> cols{};
        if (!stmt.columns.empty())
        {
            std::vector<std::string> col_names;
            for (const auto& col : stmt.columns)
                col_names.push_back(col.value);
            cols.emplace(std::move(col_names));
        }

        auto insert = std::make_unique<InsertPlanNode>(
            stmt.table.table_name.value,
            stmt.table.schema_name.has_value()
                ? stmt.table.schema_name.value().value
                : db_config_.default_schema,
            cols,
            std::move(values_node)
        );

        QueryPlan plan;
        plan.root = std::move(insert);
        plan.type = QueryPlan::Type::INSERT;
        plan.needs_stream = false;
        plan.db_specific = true;
        plan.needs_txn = true;

        return plan;
    }

    QueryPlan
    StdPlanner::plan(UpdateStmt& stmt) const
    {
        const auto* table = ssp_.ddl().get_table(stmt.table);

        const std::string schema_name = stmt.table.schema_name.has_value()
                                            ? stmt.table.schema_name.value().value
                                            : db_config_.default_schema;

        std::vector<Assignment> assignments;
        for (const auto& assignment : stmt.assignments)
        {
            if (assignment.left->type == AstNodeType::COLUMN_IDENTIFIER &&
                assignment.right->type == AstNodeType::LITERAL)
            {
                const auto& col_id =
                    table->get_column(std::get<SqlToken>(assignment.left->value)).id;
                auto data_token = DataToken(std::get<SqlToken>(assignment.right->value));

                assignments.emplace_back(std::make_pair(col_id, data_token));
            }
            if (assignment.left->type == AstNodeType::COLUMN_IDENTIFIER &&
                assignment.right->type == AstNodeType::COLUMN_IDENTIFIER)
            {
                const auto& left = table->get_column(std::get<SqlToken>(assignment.left->value));
                const auto& right = table->get_column(std::get<SqlToken>(assignment.right->value));

                assignments.emplace_back(std::make_pair(left.id, right.id));
            }
        }

        std::unique_ptr<IPlanNode> root =
            std::make_unique<SeqScanPlanNode>(stmt.table.table_name, schema_name);

        if (stmt.where)
        {
            root = std::make_unique<FilterPlanNode>(
                *ssp_.ddl().get_table(stmt.table),
                std::move(*stmt.where),
                std::move(root)
            );
        }

        auto update = std::make_unique<UpdatePlanNode>(
            stmt.table.table_name,
            schema_name,
            assignments,
            std::move(root)
        );

        QueryPlan plan;
        plan.root = std::move(update);
        plan.type = QueryPlan::Type::UPDATE;
        plan.needs_stream = false;
        plan.db_specific = true;
        plan.needs_txn = true;

        return plan;
    }

    QueryPlan
    StdPlanner::plan(DeleteStmt& stmt) const
    {
        std::string schema_name = stmt.table.schema_name.has_value()
                                      ? stmt.table.schema_name.value().value
                                      : db_config_.default_schema;

        std::unique_ptr<IPlanNode> root =
            std::make_unique<SeqScanPlanNode>(stmt.table.table_name, schema_name);

        if (stmt.where)
        {
            root = std::make_unique<FilterPlanNode>(
                *ssp_.ddl().get_table(stmt.table),
                std::move(*stmt.where),
                std::move(root)
            );
        }

        auto del =
            std::make_unique<DeletePlanNode>(stmt.table.table_name, schema_name, std::move(root));

        QueryPlan plan;
        plan.root = std::move(del);
        plan.type = QueryPlan::Type::DELETE;
        plan.needs_stream = false;
        plan.db_specific = true;
        plan.needs_txn = true;

        return plan;
    }

    QueryPlan
    StdPlanner::plan(CreateDatabaseStmt& stmt) const
    {
        std::unique_ptr<IPlanNode> root = std::make_unique<CreateDbPlanNode>(stmt.name);

        QueryPlan plan;
        plan.root = std::move(root);
        plan.type = QueryPlan::Type::CREATE_DB;
        plan.needs_stream = false;
        plan.db_specific = false;
        plan.needs_txn = false;
        return plan;
    }

    QueryPlan
    StdPlanner::plan(const CreateTableStmt& table) const
    {
        auto name = table.table.schema_name.has_value()
                        ? table.table.schema_name.value().value
                        : db_config_.default_schema;

        const auto* schema = ssp_.ddl().get_schema(name);

        std::unique_ptr<IPlanNode> root = std::make_unique<CreateTablePlanNode>(
            table.table.table_name.value,
            *schema,
            table.columns);

        QueryPlan plan;
        plan.root = std::move(root);
        plan.type = QueryPlan::Type::CREATE_TABLE;
        plan.needs_stream = false;
        plan.db_specific = true;
        plan.needs_txn = true;
        return plan;
    }

    QueryPlan
    StdPlanner::plan(const AlterTableStmt& stmt) const
    {

        auto schema_name = stmt.table.schema_name.has_value()
                               ? stmt.table.schema_name.value().value
                               : db_config_.default_schema;

        const auto* schema = ssp_.ddl().get_schema(schema_name);

        std::unique_ptr<IPlanNode> root = std::make_unique<AlterTablePlanNode>(
            stmt.table.table_name.value,
            *schema,
            stmt.operations);

        QueryPlan plan;
        plan.root = std::move(root);
        plan.type = QueryPlan::Type::ALTER_TABLE;
        plan.needs_stream = false;
        plan.db_specific = true;
        plan.needs_txn = true;
        return plan;
    }

    QueryPlan
    StdPlanner::plan(const CreateIndexStmt& stmt) const
    {
        auto schema_name = stmt.table.schema_name.has_value()
                               ? stmt.table.schema_name.value().value
                               : db_config_.default_schema;
        auto table_name = stmt.table.table_name.value;
        auto column_name = stmt.column_name.value;
        auto index_name = stmt.index_name.value;

        std::unique_ptr<IPlanNode> root = std::make_unique<CreateIndexPlanNode>(
            index_name,
            table_name,
            schema_name,
            column_name,
            stmt.is_unique,
            stmt.is_primary);

        QueryPlan plan;
        plan.root = std::move(root);
        plan.type = QueryPlan::Type::CREATE_INDEX;
        plan.needs_stream = false;
        plan.db_specific = true;
        plan.needs_txn = true;
        return plan;
    }

    QueryPlan
    StdPlanner::plan(const DropIndexStmt& stmt) const
    {

        auto schema_name = stmt.table.schema_name.has_value()
                               ? stmt.table.schema_name.value().value
                               : db_config_.default_schema;
        auto table_name = stmt.table.table_name.value;
        auto index_name = stmt.index_name.value;

        std::unique_ptr<IPlanNode> root =
            std::make_unique<DropIndexPlanNode>(index_name, table_name, schema_name);

        QueryPlan plan;
        plan.root = std::move(root);
        plan.type = QueryPlan::Type::DROP_INDEX;
        plan.needs_stream = false;
        plan.db_specific = true;
        plan.needs_txn = true;
        return plan;
    }

    QueryPlan
    StdPlanner::plan(const DropTableStmt& stmt) const
    {
        auto schema_name = stmt.table.schema_name.has_value()
                               ? stmt.table.schema_name.value().value
                               : db_config_.default_schema;
        auto table_name = stmt.table.table_name.value;

        std::unique_ptr<IPlanNode> root =
            std::make_unique<DropTablePlanNode>(table_name, schema_name);

        QueryPlan plan;
        plan.root = std::move(root);
        plan.type = QueryPlan::Type::DROP_TABLE;
        plan.needs_stream = false;
        plan.db_specific = true;
        plan.needs_txn = true;
        return plan;
    }
} // namespace exq