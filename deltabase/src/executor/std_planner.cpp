//
// Created by poproshaikin on 09.11.25.
//

#include "include/std_planner.hpp"

#include "meta_schema.hpp"

#include <cmath>
#include <format>

namespace exq
{
    using namespace types;

    StdPlanner::StdPlanner(const Config& db_config, storage::IDbInstance& db)
        : db_(db), db_config_(db_config)
    {
    }

    QueryPlan
    StdPlanner::plan(AstNode&& ast)
    {
        if (ast.type == AstNodeType::SELECT)
        {
            return plan(std::get<SelectStatement>(ast.value));
        }
        if (ast.type == AstNodeType::INSERT)
        {
            return plan(std::get<InsertStatement>(ast.value));
        }
        if (ast.type == AstNodeType::UPDATE)
        {
            return plan(std::get<UpdateStatement>(ast.value));
        }
        if (ast.type == AstNodeType::DELETE)
        {
            return plan(std::get<DeleteStatement>(ast.value));
        }
        if (ast.type == AstNodeType::CREATE_DATABASE)
        {
            return plan(std::get<CreateDbStatement>(ast.value));
        }
        if (ast.type == AstNodeType::CREATE_TABLE)
        {
            return plan(std::get<CreateTableStatement>(ast.value));
        }
        if (ast.type == AstNodeType::CREATE_INDEX)
        {
            return plan(std::get<CreateIndexStatement>(ast.value));
        }
        if (ast.type == AstNodeType::DROP_INDEX)
        {
            return plan(std::get<DropIndexStatement>(ast.value));
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
               std::get<SqlLiteral>(token.detail) == SqlLiteral::NULL_;
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
        const MetaTable& table, const BinaryExpr* condition, const MetaIndex** chosen_index
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
    StdPlanner::plan(SelectStatement& stmt) const
    {
        auto table = db_.get_table(stmt.table);
        const auto* condition_ptr = stmt.where ? &(*stmt.where) : nullptr;

        const MetaIndex* chosen_index = nullptr;
        auto scan_type = choose_scan_type(*table, condition_ptr, &chosen_index);

        std::unique_ptr<IPlanNode> node;

        if (scan_type == IPlanNode::Type::INDEX_SCAN)
        {
            node = std::make_unique<IndexScanPlanNode>(
                stmt.table.table_name,
                stmt.table.schema_name.has_value() ? stmt.table.schema_name.value().value
                                                   : db_config_.default_schema,
                chosen_index->id,
                std::move(*stmt.where)
            );
        }
        else
        {
            node = std::make_unique<SeqScanPlanNode>(
                stmt.table.table_name,
                stmt.table.schema_name.has_value() ? stmt.table.schema_name.value().value
                                                   : db_config_.default_schema
            );

            // 2. WHERE
            if (stmt.where)
            {
                auto filter = std::make_unique<FilterPlanNode>(
                    *db_.get_table(stmt.table), std::move(*stmt.where), std::move(node)
                );
                node = std::move(filter);
            }
        }

        // 3. PROJECT
        if (!stmt.columns.empty())
        {
            std::vector<std::string> cols;
            for (auto& c : stmt.columns)
                cols.push_back(c.value);

            auto project = std::make_unique<ProjectPlanNode>(
                *db_.get_table(stmt.table), cols, std::move(node)
            );
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
        plan.needs_stream = db_.needs_stream(*node);
        plan.root = std::move(node);
        plan.db_specific = true;
        return plan;
    }

    QueryPlan
    StdPlanner::plan(InsertStatement& stmt) const
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
            stmt.table.schema_name.has_value() ? stmt.table.schema_name.value().value
                                               : db_config_.default_schema,
            cols,
            std::move(values_node)
        );

        QueryPlan plan;
        plan.root = std::move(insert);
        plan.type = QueryPlan::Type::INSERT;
        plan.needs_stream = false;
        plan.db_specific = true;

        return plan;
    }

    QueryPlan
    StdPlanner::plan(UpdateStatement& stmt) const
    {
        const auto* table = db_.get_table(stmt.table);

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
                *db_.get_table(stmt.table), std::move(*stmt.where), std::move(root)
            );
        }

        auto update = std::make_unique<UpdatePlanNode>(
            stmt.table.table_name, schema_name, assignments, std::move(root)
        );

        QueryPlan plan;
        plan.root = std::move(update);
        plan.type = QueryPlan::Type::UPDATE;
        plan.needs_stream = false;
        plan.db_specific = true;

        return plan;
    }

    QueryPlan
    StdPlanner::plan(DeleteStatement& stmt) const
    {
        std::string schema_name = stmt.table.schema_name.has_value()
                                      ? stmt.table.schema_name.value().value
                                      : db_config_.default_schema;

        std::unique_ptr<IPlanNode> root =
            std::make_unique<SeqScanPlanNode>(stmt.table.table_name, schema_name);

        if (stmt.where)
        {
            root = std::make_unique<FilterPlanNode>(
                *db_.get_table(stmt.table), std::move(*stmt.where), std::move(root)
            );
        }

        auto del =
            std::make_unique<DeletePlanNode>(stmt.table.table_name, schema_name, std::move(root));

        QueryPlan plan;
        plan.root = std::move(del);
        plan.type = QueryPlan::Type::DELETE;
        plan.needs_stream = false;
        plan.db_specific = true;

        return plan;
    }

    QueryPlan
    StdPlanner::plan(CreateDbStatement& stmt) const
    {
        std::unique_ptr<IPlanNode> root = std::make_unique<CreateDbPlanNode>(stmt.name);

        QueryPlan plan;
        plan.root = std::move(root);
        plan.type = QueryPlan::Type::CREATE_DB;
        plan.needs_stream = false;
        plan.db_specific = false;
        return plan;
    }

    QueryPlan
    StdPlanner::plan(const CreateTableStatement& table) const
    {
        auto name = table.table.schema_name.has_value() ? table.table.schema_name.value().value
                                                        : db_config_.default_schema;

        const auto* schema = db_.get_schema(name);

        std::unique_ptr<IPlanNode> root = std::make_unique<CreateTablePlanNode>(
            table.table.table_name.value, *schema, table.columns
        );

        QueryPlan plan;
        plan.root = std::move(root);
        plan.type = QueryPlan::Type::CREATE_TABLE;
        plan.needs_stream = false;
        plan.db_specific = true;
        return plan;
    }

    QueryPlan
    StdPlanner::plan(const CreateIndexStatement& stmt) const
    {
        auto schema_name = stmt.table.schema_name.has_value() ? stmt.table.schema_name.value().value
                                                              : db_config_.default_schema;
        auto table_name = stmt.table.table_name.value;
        auto column_name = stmt.column_name.value;
        auto index_name = stmt.index_name.value;

        std::unique_ptr<IPlanNode> root = std::make_unique<CreateIndexPlanNode>(
            index_name, table_name, schema_name, column_name, stmt.is_unique
        );

        QueryPlan plan;
        plan.root = std::move(root);
        plan.type = QueryPlan::Type::CREATE_INDEX;
        plan.needs_stream = false;
        plan.db_specific = true;
        return plan;
    }

    QueryPlan
    StdPlanner::plan(const DropIndexStatement& stmt) const
    {

        auto schema_name = stmt.table.schema_name.has_value() ? stmt.table.schema_name.value().value
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
        return plan;
    }
} // namespace exq