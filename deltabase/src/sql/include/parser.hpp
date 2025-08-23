#ifndef SQL_PARSER_HPP
#define SQL_PARSER_HPP

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
        DELETE,
        CREATE_TABLE,
    };

    enum class AstOperator {
        OR = 1,
        AND,
        NOT,

        EQ, NEQ,

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
            { AstOperator::NEQ, 4 },
            { AstOperator::GR, 4 },
            { AstOperator::GRE, 4 },
            { AstOperator::LT, 4 },
            { AstOperator::LTE, 4 },
            { AstOperator::ASSIGN, 4 },
        };

        return map;
    }

    struct AstNode;
    using AstNodePtr = std::unique_ptr<AstNode>;

    struct BinaryExpr {
        AstOperator op;
        AstNodePtr left;
        AstNodePtr right;
    };

    struct SelectStatement {
        std::vector<SqlToken> columns;
        SqlToken table;
        AstNodePtr where;
    };

    struct InsertStatement {
        SqlToken table;
        std::vector<SqlToken> columns;
        std::vector<SqlToken> values;
    };

    struct UpdateStatement {
        SqlToken table;
        std::vector<AstNode> assignments;
        AstNodePtr where;
    };

    struct DeleteStatement {
        SqlToken table;
        AstNodePtr where;
    };

    struct ColumnDefinition {
        AstNodePtr name;
        AstNodePtr type;
        std::vector<AstNodePtr> constraints;
    };

    struct CreateTableStatement {
        AstNodePtr name;
        std::vector<AstNodePtr> columns;
    };

    using AstNodeValue = std::variant<
        SqlToken,
        BinaryExpr,
        SelectStatement,
        InsertStatement,
        UpdateStatement,
        DeleteStatement,
        CreateTableStatement,
        ColumnDefinition
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
            CreateTableStatement parse_create_table();

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

#endif
