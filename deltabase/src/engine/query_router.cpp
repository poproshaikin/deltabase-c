#include "include/query_router.hpp"
#include "../executor/include/semantic_analyzer.hpp"
#include "../sql/include/parser.hpp"
#include "../catalog/include/meta_registry.hpp"

namespace engine {
    QueryRouter::QueryRouter(catalog::MetaRegistry& registry, const EngineConfig& cfg)
        : registry_(registry), cfg_(cfg) {
        db_executor_.emplace(registry, cfg);
        virtual_executor_.emplace(registry, cfg);
        admin_executor_.emplace(registry, cfg);
    }

    auto
    QueryRouter::route_and_execute(const sql::AstNode& node, const exe::AnalysisResult& analysis) -> exe::IntOrDataTable {
        if (!this->can_execute(node, analysis)) {
            throw std::runtime_error("QueryRouter: No suitable executor available for this query type");
        }
        
        switch (node.type) {
        case sql::AstNodeType::SELECT: {
            const auto& stmt = std::get<sql::SelectStatement>(node.value);
            
            if (catalog::is_table_virtual(stmt.table)) {
                return virtual_executor_.value().execute(node);
            }
            
            return db_executor_.value().execute(node);
        }
        case sql::AstNodeType::INSERT:
        case sql::AstNodeType::UPDATE:
        case sql::AstNodeType::DELETE:
        case sql::AstNodeType::CREATE_TABLE:
            return db_executor_.value().execute(node);

        case sql::AstNodeType::CREATE_DATABASE:
            return admin_executor_.value().execute(node);
            
        default:
            throw std::runtime_error("QueryRouter: Unsupported query type");
        }
    }

    auto
    QueryRouter::can_execute(const sql::AstNode& node, const exe::AnalysisResult& analysis) -> bool {
        switch (node.type) {
        case sql::AstNodeType::SELECT: {
            const auto& stmt = std::get<sql::SelectStatement>(node.value);
            
            if (catalog::is_table_virtual(stmt.table)) {
                return virtual_executor_.has_value();
            }

            return db_executor_.has_value();
        }
        case sql::AstNodeType::INSERT:
        case sql::AstNodeType::UPDATE:
        case sql::AstNodeType::DELETE:
        case sql::AstNodeType::CREATE_TABLE:
            return db_executor_.has_value();
        case sql::AstNodeType::CREATE_DATABASE:
            return admin_executor_.has_value();
        default:
            throw std::runtime_error("Invalid AST node was passed to QueryRouter::can_execute");
        }
    }

}