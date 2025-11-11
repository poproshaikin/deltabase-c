//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_QUERY_PLAN_HPP
#define DELTABASE_QUERY_PLAN_HPP
#include "ast_tree.hpp"
#include "data_row.hpp"

#include <string>
#include <variant>

namespace types
{
    struct FromPlanNode;
    struct FilterPlanNode;
    struct ProjectPlanNode;
    struct LimitPlanNode;
    struct InsertPlanNode;
    struct ValuesPlanNode;
    struct SeqScanPlanNode;

    using QueryPlanNode = std::variant<
        FromPlanNode,
        FilterPlanNode,
        ProjectPlanNode,
        LimitPlanNode,
        InsertPlanNode,
        ValuesPlanNode,
        SeqScanPlanNode
    >;

    struct SeqScanPlanNode
    {
        std::string table_name;
        std::optional<std::string> schema_name;
    };

    struct FromPlanNode
    {
        std::string table_name;
        std::optional<std::string> schema_name;
        std::unique_ptr<QueryPlanNode> child;
    };

    struct FilterPlanNode
    {
        BinaryExpr where;
        std::unique_ptr<QueryPlanNode> child;
    };

    struct ProjectPlanNode
    {
        std::vector<std::string> columns;
        std::unique_ptr<QueryPlanNode> child;
    };

    struct LimitPlanNode
    {
        uint64_t limit;
        std::unique_ptr<QueryPlanNode> child;
    };

    struct ValuesPlanNode
    {
        std::vector<DataRow> values;
    };

    struct InsertPlanNode
    {
        std::string table_name;
        std::optional<std::string> schema_name;
        std::optional<std::vector<std::string> > column_names;

        std::unique_ptr<QueryPlanNode> child;
    };
}

#endif //DELTABASE_QUERY_PLAN_HPP