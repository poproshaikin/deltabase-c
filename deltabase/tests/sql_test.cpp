#include <iostream>
#include <string>
#include <vector>

#include "../src/executor/include/semantic_analyzer.hpp"

using namespace sql;

// int main() {
//     std::cout << "here";
// }
void print_ast_node(const std::unique_ptr<AstNode>& node, int indent = 0) {
    if (!node) {
        std::cout << std::string(indent, ' ') << "(null)\n";
        return;
    }

    std::string indent_str(indent, ' ');
    std::cout << indent_str << "Node type: " << static_cast<int>(node->type) << "\n";

    std::visit([&](auto&& value) {
        using T = std::decay_t<decltype(value)>;

        if constexpr (std::is_same_v<T, SqlToken>) {
            std::cout << indent_str << "  SqlToken: " << value.to_string(indent + 6) << "\n";
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
            std::cout << indent_str << "  BinaryExpr:\n";
            std::cout << indent_str << "    Operator: " << static_cast<int>(value.op) << "\n";
            std::cout << indent_str << "    Left:\n";
            print_ast_node(value.left, indent + 6);
            std::cout << indent_str << "    Right:\n";
            print_ast_node(value.right, indent + 6);
        } else if constexpr (std::is_same_v<T, SelectStatement>) {
            std::cout << indent_str << "  SelectStatement:\n";
            std::cout << indent_str << "    Table:\n";
            print_ast_node(value.table, indent + 6);
            std::cout << indent_str << "    Columns:\n";
            for (const auto& col : value.columns) {
                print_ast_node(col, indent + 6);
            }
            if (value.where) {
                std::cout << indent_str << "    Where:\n";
                print_ast_node(value.where, indent + 6);
            }
        } else if constexpr (std::is_same_v<T, UpdateStatement>) {
            std::cout << indent_str << "  UpdateStatement:\n";
            std::cout << indent_str << "    Table:\n";
            print_ast_node(value.table, indent + 6);

            std::cout << indent_str << "    Assignments:\n";
            for (const auto& assign : value.assignments) {
                print_ast_node(assign, indent + 6);
            }

            std::cout << indent_str << "    Where:\n";
            if (value.where) {
                print_ast_node(value.where, indent + 6);
            } else {
                std::cout << indent_str << "      (none)\n";
            }

        } else if constexpr (std::is_same_v<T, DeleteStatement>) {
            std::cout << indent_str << "  DeleteStatement:\n";
            std::cout << indent_str << "    Table:\n";
            print_ast_node(value.table, indent + 6);

            std::cout << indent_str << "    Where:\n";
            if (value.where) {
                print_ast_node(value.where, indent + 6);
            } else {
                std::cout << indent_str << "      (none)\n";
            }
        }
        else if constexpr (std::is_same_v<T, InsertStatement>) {
            std::cout << indent_str << "  InsertStatement:\n";
            std::cout << indent_str << "    Table:\n";
            print_ast_node(value.table, indent + 6);
            std::cout << indent_str << "    Columns:\n";
            for (const auto& col : value.columns) {
                print_ast_node(col, indent + 6);
            }
            std::cout << indent_str << "    Values:\n";
            for (const auto& val : value.values) {
                print_ast_node(val, indent + 6);
            }
        } else {
            std::cout << indent_str << "  (unknown node type)\n";
        }
    }, node->value);
}
int main() {
    std::string db_name = "testdb";
    std::string sql = "DELETE FROM users WHERE id == 5";
    std::cout << sql << std::endl;
    //
    sql::SqlTokenizer tokenizer;
    std::vector<sql::SqlToken> result = tokenizer.tokenize(sql);

    sql::SqlParser parser(result);
    std::unique_ptr<sql::AstNode> node = parser.parse();
    if (!node) {
        std::cerr << "parser.parse() вернул nullptr!\n";
        return 1;
    }

    // // print_ast_node(node);

    exe::SemanticAnalyzer analyzer(db_name);
    analyzer.analyze(node.get());
    //
}
