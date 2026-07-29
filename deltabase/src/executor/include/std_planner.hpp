//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_SIMPLE_PLANNER_HPP
#define DELTABASE_SIMPLE_PLANNER_HPP
#include "information_schema_provider.hpp"
#include "planner.hpp"
#include "../../types/include/config.hpp"
#include "../../storage/include/storage_service_provider.hpp"

namespace exq
{
    class StdPlanner final : public IPlanner
    {
        storage::StorageServiceProvider& ssp_;
        types::Config db_config_;
        InformationSchemaProvider info_schema_provider_;

        static double
        estimate_seq_scan_selectivity(const types::MetaTable& table, const types::IPlanNode& node);

        static bool
        should_stream_for_seq_scan(const types::MetaTable& table, const types::IPlanNode& node);

        types::QueryPlan
        plan(types::SelectStmt& stmt) const;

        types::QueryPlan
        plan(types::InsertStmt& stmt) const;

        types::QueryPlan
        plan(types::UpdateStmt& stmt) const;

        types::QueryPlan
        plan(types::DeleteStmt& stmt) const;

        types::QueryPlan
        plan(types::CreateDatabaseStmt& stmt) const;

        types::QueryPlan
        plan(const types::CreateTableStmt& table) const;

        types::QueryPlan
        plan(const types::AlterTableStmt& stmt) const;

        types::QueryPlan
        plan(const types::CreateIndexStmt& stmt) const;

        types::QueryPlan
        plan(const types::DropIndexStmt& stmt) const;

        types::QueryPlan
        plan(const types::DropTableStmt& stmt) const;

    public:
        explicit
        StdPlanner(const types::Config& db_config, storage::StorageServiceProvider& ssp);

        types::QueryPlan
        plan(types::AstNode&& ast) override;
    };
}

#endif //DELTABASE_SIMPLE_PLANNER_HPP