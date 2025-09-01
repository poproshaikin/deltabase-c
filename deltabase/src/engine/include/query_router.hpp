#pragma once

#include "../../executor/include/query_executor.hpp"
#include "../../executor/include/semantic_analyzer.hpp"
#include "meta_registry.hpp"

namespace engine {
    class QueryRouter {
        catalog::MetaRegistry& registry_;
        std::optional<std::string> db_name_;
        
        std::optional<exe::DatabaseExecutor> db_executor_;
        std::optional<exe::VirtualExecutor> virtual_executor_;
        std::optional<exe::AdminExecutor> admin_executor_;

    public:
        QueryRouter(catalog::MetaRegistry& registry) : registry_(registry), db_name_(std::nullopt) {
            virtual_executor_.emplace(registry);
            admin_executor_.emplace(registry, std::nullopt);
        }

        QueryRouter(catalog::MetaRegistry& registry, std::string db_name)
            : registry_(registry), db_name_(db_name) {
            db_executor_.emplace(registry, db_name);
            virtual_executor_.emplace(registry);
            admin_executor_.emplace(registry, db_name);
        }

        auto 
        can_execute(const sql::AstNode& node, const exe::AnalysisResult& analysis) -> bool;

        auto
        route_and_execute(const sql::AstNode& node, const exe::AnalysisResult& analysis) -> exe::IntOrDataTable;
    };
}