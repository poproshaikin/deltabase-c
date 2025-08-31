#pragma once

#include "../../executor/include/query_executor.hpp"
#include "../../executor/include/semantic_analyzer.hpp"
#include "meta_registry.hpp"

namespace engine {
    class QueryRouter {
        catalog::MetaRegistry& registry;
        std::optional<std::string> db_name;
        
        std::optional<exe::DatabaseExecutor> db_executor;
        std::optional<exe::VirtualExecutor> virtual_executor;
        std::optional<exe::AdminExecutor> admin_executor;

    public:
        QueryRouter(catalog::MetaRegistry& registry) : registry(registry), db_name(std::nullopt) {
            virtual_executor.emplace(registry);
            admin_executor.emplace(registry, std::nullopt);
        }

        QueryRouter(catalog::MetaRegistry& registry, std::string db_name)
            : registry(registry), db_name(db_name) {
            db_executor.emplace(registry, db_name);
            virtual_executor.emplace(registry);
            admin_executor.emplace(registry, db_name);
        }

        bool 
        can_execute(const sql::AstNode& node, const exe::AnalysisResult& analysis);

        exe::IntOrDataTable
        route_and_execute(const sql::AstNode& node, const exe::AnalysisResult& analysis);
    };
}