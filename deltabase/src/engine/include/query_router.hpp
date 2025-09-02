#pragma once

#include "../../executor/include/query_executor.hpp"
#include "../../executor/include/semantic_analyzer.hpp"
#include "config.hpp"
#include "meta_registry.hpp"

namespace engine {
    class QueryRouter {
        catalog::MetaRegistry& registry_;
        const EngineConfig& cfg_;
        
        std::optional<exe::DatabaseExecutor> db_executor_;
        std::optional<exe::VirtualExecutor> virtual_executor_;
        std::optional<exe::AdminExecutor> admin_executor_;

    public:
        QueryRouter(catalog::MetaRegistry& registry, const EngineConfig& cfg);

        auto 
        can_execute(const sql::AstNode& node, const exe::AnalysisResult& analysis) -> bool;
        auto
        route_and_execute(const sql::AstNode& node, const exe::AnalysisResult& analysis) -> exe::IntOrDataTable;
    };
}