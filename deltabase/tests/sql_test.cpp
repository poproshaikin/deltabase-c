#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>
#include <variant>
#include <vector>

#include "../src/executor/include/semantic_analyzer.hpp"
#include "../src/executor/include/query_executor.hpp"

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

std::string token_to_string(const DataToken* token) {
    if (!token || token->type == DT_NULL) {
        return "NULL";
    }

    switch (token->type) {
        case DT_INTEGER: {
            int32_t value;
            memcpy(&value, token->bytes, sizeof(value));
            return std::to_string(value);
        }
        case DT_REAL: {
            double value;
            memcpy(&value, token->bytes, sizeof(value));
            return std::to_string(value);
        }
        case DT_BOOL: {
            bool value;
            memcpy(&value, token->bytes, sizeof(value));
            return value ? "true" : "false";
        }
        case DT_CHAR: {
            return std::string(1, *token->bytes);  // 1 символ
        }
        case DT_STRING: {
            return std::string(token->bytes, token->size);
        }
        default:
            return "<unknown>";
    }
}

void print_data_table(const DataTable* table) {
    if (!table || !table->scheme) {
        std::cerr << "Invalid DataTable\n";
        return;
    }

    const MetaTable* schema = table->scheme;

    // Заголовки
    for (size_t i = 0; i < schema->columns_count; ++i) {
        std::cout << std::left << std::setw(15) << schema->columns[i]->name;
    }
    std::cout << "\n";

    // Разделитель
    for (size_t i = 0; i < schema->columns_count; ++i) {
        std::cout << std::setw(15) << std::setfill('-') << "";
    }
    std::cout << std::setfill(' ') << "\n";

    // Данные
    for (size_t r = 0; r < table->rows_count; ++r) {
        const DataRow* row = table->rows[r];
        for (size_t c = 0; c < schema->columns_count; ++c) {
            const DataToken* token = row->tokens[c];
            std::cout << std::left << std::setw(15) << token_to_string(token);
        }
        std::cout << "\n";
    }
}

int main() {
    std::string db_name = "testdb";

    // std::string sql = "INSERT INTO users(id, name) VALUES(5, 'hello')";
    std::string sql = "SELECT * FROM users";
    // std::string sql = "UPDATE users SET email = 'fukcking' WHERE id == 2";

    std::cout << sql << std::endl;

    sql::SqlTokenizer tokenizer;
    std::vector<sql::SqlToken> tokens = tokenizer.tokenize(sql);

    sql::SqlParser parser(tokens);
    std::unique_ptr<sql::AstNode> node = parser.parse();

    exe::SemanticAnalyzer analyzer(db_name);
    analyzer.analyze(node.get());

    exe::QueryExecutor executor(db_name);
    const auto result = executor.execute(*node);

    if (std::holds_alternative<std::unique_ptr<DataTable>>(result))
        print_data_table(std::get<std::unique_ptr<DataTable>>(result).get());
}
