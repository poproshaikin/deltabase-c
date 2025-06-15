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

    enum class AstOperator {
        OR = 1,
        AND,
        NOT,

        GR, 
        GRE,
        LT,
        LTE,
    };

    inline const std::unordered_map<AstOperator, int>& getAstOperatorsPriorities() {
        static std::unordered_map<AstOperator, int> map = {
            { AstOperator::OR, 1 },
            { AstOperator::AND, 2 },
            { AstOperator::NOT, 3 },
            { AstOperator::GR, 4 },
            { AstOperator::GRE, 4 },
            { AstOperator::LT, 4 },
            { AstOperator::LTE, 4 },
        };

        return map;
    }

    struct AstNode;

    struct BinaryExpr {
        AstOperator op;
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

    struct UpdateStatement {
        std::unique_ptr<AstNode> table;
        std::vector<std::unique_ptr<AstNode>> assignments;
        std::unique_ptr<AstNode> where;
    };

    struct DeleteStatement {
        std::unique_ptr<AstNode> table;
        std::unique_ptr<AstNode> where;
    };

    using AstNodeValue = std::variant<
        SqlToken,
        BinaryExpr,
        SelectStatement,
        InsertStatement
    >;

    struct AstNode {
        AstNodeType type;
        AstNodeValue value;

        AstNode() = default;
        AstNode(AstNodeType type, AstNodeValue&& value);
    };

    class SqlParser {
        public:
            SqlParser(std::vector<SqlToken> tokens);
            std::unique_ptr<AstNode> parse();

        private: 
            const std::vector<SqlToken> _tokens;
            size_t _current;
            
            SelectStatement parse_select();
            InsertStatement parse_insert();

            std::unique_ptr<AstNode> parse_binary(int min_priority);

            bool match(SqlTokenType type) const noexcept;
            bool match(SqlKeyword kw) const noexcept;
            bool match(SqlOperator op) const noexcept;
            bool match(SqlSymbol sym) const noexcept;

            bool advance() noexcept;
            const SqlToken& previous() const noexcept;
            const SqlToken& current() const noexcept;

            std::vector<std::unique_ptr<AstNode>> parse_columns();
            std::unique_ptr<AstNode> parse_primary();
    };
}
