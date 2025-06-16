#include "../../sql/include/parser.hpp"

namespace exe {
    class SemanticAnalyzer {
        public:
            void analyze(const sql::AstNode& ast);
        private:
            void analyze_select(const sql::SelectStatement& selectStmt);
            void analyze_insert(const sql::InsertStatement& insertStmt);
            void analyze_update(const sql::UpdateStatement& updateStmt);
            void analyze_delete(const sql::DeleteStatement& deleteStmt);
    };
}
