#include "include/query_router.hpp"
#include "../executor/include/semantic_analyzer.hpp"
#include "../sql/include/parser.hpp"
#include "../catalog/include/meta_registry.hpp"

namespace engine {
    exe::IntOrDataTable
    QueryRouter::route_and_execute(const sql::AstNode& node, const exe::AnalysisResult& analysis) {
        if (!this->can_execute(node, analysis)) {
            throw std::runtime_error("QueryRouter: No suitable executor available for this query type");
        }
        
        switch (node.type) {
        case sql::AstNodeType::SELECT: {
            const sql::SelectStatement& stmt = std::get<sql::SelectStatement>(node.value);
            
            if (catalog::is_table_virtual(stmt.table)) {
                return virtual_executor->execute(node);
            }
            
            return db_executor->execute(node);
        }
        case sql::AstNodeType::INSERT:
        case sql::AstNodeType::UPDATE:
        case sql::AstNodeType::DELETE:
        case sql::AstNodeType::CREATE_TABLE:
            return db_executor->execute(node);
            
        case sql::AstNodeType::CREATE_DATABASE:
            return admin_executor->execute(node);
            
        default:
            throw std::runtime_error("QueryRouter: Unsupported query type");
        }
    }

    bool
    QueryRouter::can_execute(const sql::AstNode& node, const exe::AnalysisResult& analysis) {
        switch (node.type) {
        case sql::AstNodeType::SELECT: {
            const sql::SelectStatement& stmt = std::get<sql::SelectStatement>(node.value);
            
            if (catalog::is_table_virtual(stmt.table)) {
                return this->virtual_executor.has_value();
            }

            return this->db_executor.has_value();
        }
        case sql::AstNodeType::INSERT:
        case sql::AstNodeType::UPDATE:
        case sql::AstNodeType::DELETE:
        case sql::AstNodeType::CREATE_TABLE:
            return this->db_executor.has_value();
        case sql::AstNodeType::CREATE_DATABASE:
            return this->admin_executor.has_value();
        default:
            throw std::runtime_error("Invalid AST node was passed to QueryRouter::can_execute");
        }
    }

}