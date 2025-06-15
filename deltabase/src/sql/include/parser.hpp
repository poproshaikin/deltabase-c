#include "lexer.hpp"
#include <memory>

namespace sql {
    enum class AstNodeType {
        IDENTIFIER,
        TABLE_IDENTIFIER,
        COLUMN_IDENTIFIER,
        LITERAL,
        BINARY_EXPR,
        SELECT,
        INSERT,
        UPDATE,
        DELETE
    };

    struct AstNode;


    struct BinaryExpr {
        SqlOperator op;
        std::unique_ptr<AstNode> left;
        std::unique_ptr<AstNode> right;
    };

    struct SelectStatement {
        std::unique_ptr<AstNode> table;
        std::vector<std::unique_ptr<AstNode>> columns;
        std::unique_ptr<AstNode> where;
    };

    struct InsertStatement {
        std::unique_ptr<AstNode> table;
        std::vector<std::unique_ptr<AstNode>> columns;
        std::vector<std::unique_ptr<AstNode>> values;
    };

    struct AstNode {
        using Variant = std::variant<
            BinaryExpr,
            SelectStatement,
            InsertStatement
        >;

        AstNodeType type;
        Variant value;
    };
}
