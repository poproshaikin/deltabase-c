#include "lexer.hpp"
#include <memory>

namespace sql {
    enum class AstNodeType {
        IDENTIFIER = 1,
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

        EQ,

        GR, 
        GRE,
        LT,
        LTE,

        ASSIGN,
    };

    inline const std::unordered_map<AstOperator, int>& getAstOperatorsPriorities() {
        static std::unordered_map<AstOperator, int> map = {
            { AstOperator::OR, 1 },
            { AstOperator::AND, 2 },
            { AstOperator::NOT, 3 },
            { AstOperator::EQ, 4 },
            { AstOperator::GR, 4 },
            { AstOperator::GRE, 4 },
            { AstOperator::LT, 4 },
            { AstOperator::LTE, 4 },
            { AstOperator::ASSIGN, 4 },
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
        InsertStatement,
        UpdateStatement,
        DeleteStatement
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
            std::vector<SqlToken> _tokens;
            size_t _current;
            
            SelectStatement parse_select();
            InsertStatement parse_insert();
            UpdateStatement parse_update();
            DeleteStatement parse_delete();

            std::unique_ptr<AstNode> parse_binary(int min_priority);

            template<typename TEnum>
            bool match(const TEnum&) const ;
            template<typename TEnum>
            bool match_or_throw(TEnum expected, std::string error_msg) const;


            bool advance() noexcept;
            bool advance_or_throw(std::string error_msg = "Invalid statement syntax");
            const SqlToken& previous() const noexcept;
            const SqlToken& current() const;

            std::unique_ptr<AstNode> parse_primary();
            std::vector<std::unique_ptr<AstNode>> parse_assignments();
            std::vector<std::unique_ptr<AstNode>> parse_tokens_list(SqlTokenType tokenType, AstNodeType nodeType);
    };
}
