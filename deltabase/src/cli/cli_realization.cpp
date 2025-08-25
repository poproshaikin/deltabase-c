#include "include/cli.hpp"
#include "../sql/include/lexer.hpp"
#include "../sql/include/parser.hpp"
#include "../executor/include/semantic_analyzer.hpp"
#include "../executor/include/query_executor.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>
#include "../engine.hpp"

// Forward declarations for print functions
void print_ast_node(const std::unique_ptr<sql::AstNode>& node, int indent);
void print_ast_node(const sql::AstNode& node, int indent);
void print_sql_token(const sql::SqlToken& token, int indent);

void print_sql_token(const sql::SqlToken& token, int indent) {
    std::string indent_str(indent, ' ');
    std::cout << indent_str << "SqlToken: " << token.to_string() << "\n";
}

void print_ast_node(const sql::AstNode& node, int indent) {
    std::string indent_str(indent, ' ');
    std::cout << indent_str << "Node type: " << static_cast<int>(node.type) << "\n";

    std::visit([&](auto&& value) {
            using T = std::decay_t<decltype(value)>;

            if constexpr (std::is_same_v<T, sql::SqlToken>) {
                print_sql_token(value, indent + 2);
            } else if constexpr (std::is_same_v<T, sql::BinaryExpr>) {
                std::cout << indent_str << "  BinaryExpr:\n";
                std::cout << indent_str << "    Operator: " << static_cast<int>(value.op) << "\n";
                std::cout << indent_str << "    Left:\n";
                print_ast_node(value.left, indent + 6);
                std::cout << indent_str << "    Right:\n";
                print_ast_node(value.right, indent + 6);
            } else if constexpr (std::is_same_v<T, sql::SelectStatement>) {
                std::cout << indent_str << "  SelectStatement:\n";
                std::cout << indent_str << "    Table:\n";
                print_sql_token(value.table, indent + 6);
                std::cout << indent_str << "    Columns:\n";
                for (const auto& col : value.columns) {
                    print_sql_token(col, indent + 6);
                }
                if (value.where) {
                    std::cout << indent_str << "    Where:\n";
                    print_ast_node(value.where, indent + 6);
                }
            } else if constexpr (std::is_same_v<T, sql::UpdateStatement>) {
                std::cout << indent_str << "  UpdateStatement:\n";
                std::cout << indent_str << "    Table:\n";
                print_sql_token(value.table, indent + 6);

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

            } else if constexpr (std::is_same_v<T, sql::DeleteStatement>) {
                std::cout << indent_str << "  DeleteStatement:\n";
                std::cout << indent_str << "    Table:\n";
                print_sql_token(value.table, indent + 6);

                std::cout << indent_str << "    Where:\n";
                if (value.where) {
                    print_ast_node(value.where, indent + 6);
                } else {
                    std::cout << indent_str << "      (none)\n";
                }
            }
            else if constexpr (std::is_same_v<T, sql::InsertStatement>) {
                std::cout << indent_str << "  InsertStatement:\n";
                std::cout << indent_str << "    Table:\n";
                print_sql_token(value.table, indent + 6);
                std::cout << indent_str << "    Columns:\n";
                for (const auto& col : value.columns) {
                    print_sql_token(col, indent + 6);
                }
                std::cout << indent_str << "    Values:\n";
                for (const auto& val : value.values) {
                    print_sql_token(val, indent + 6);
                }
            } else if constexpr (std::is_same_v<T, sql::CreateTableStatement>) {
                std::cout << indent_str << "  CreateTableStatement:\n";
                std::cout << indent_str << "    Table:\n";
                print_sql_token(value.name, indent + 6);
                std::cout << indent_str << "    Columns:\n";
                for (const auto& col : value.columns) {
                    //print_ast_node(col, indent + 6);
                }
            } else if constexpr (std::is_same_v<T, sql::ColumnDefinition>) {
                std::cout << indent_str << "  ColumnDefinition:\n";
                std::cout << indent_str << "    Name:\n";
                print_sql_token(value.name, indent + 6);
                std::cout << indent_str << "    Type:\n";
                print_sql_token(value.type, indent + 6);
                if (!value.constraints.empty()) {
                    std::cout << indent_str << "    Constraints:\n";
                    for (const auto& constraint : value.constraints) {
                        print_sql_token(constraint, indent + 6);
                    }
                }
            } else {
                std::cout << indent_str << "  (unknown node type)\n";
            }
    }, node.value);
}

void print_ast_node(const std::unique_ptr<sql::AstNode>& node, int indent) {
    if (!node) {
        std::cout << std::string(indent, ' ') << "(null)\n";
        return;
    }
    print_ast_node(*node, indent);
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

    for (size_t i = 0; i < schema->columns_count; ++i) {
        std::cout << std::left << std::setw(15) << schema->columns[i]->name;
    }
    std::cout << "\n";

    for (size_t i = 0; i < schema->columns_count; ++i) {
        std::cout << std::setw(15) << std::setfill('-') << "";
    }
    std::cout << std::setfill(' ') << "\n";

    for (size_t r = 0; r < table->rows_count; ++r) {
        const DataRow* row = table->rows[r];
        for (size_t c = 0; c < schema->columns_count; ++c) {
            const DataToken* token = row->tokens[c];
            std::cout << std::left << std::setw(15) << token_to_string(token);
        }
        std::cout << "\n";
    }
}

void cli::SqlCli::run_query_console() {
    std::string input;
    std::cout << "Welcome to deltabase! Type 'exit' to quit.\n";
    std::cout << "Enter database name: ";
    std::cin >> input;
    std::cout << std::endl;
    
    DltEngine engine(input);

    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, input)) {
            break;
        }
        if (input == "exit") {
            break;
        }

        const ExecutionResult& result = engine.run(input);

        std::cout << "Execution time: " << result.execution_time_ms << "ms" << std::endl;

        if (std::holds_alternative<std::unique_ptr<DataTable>>(result.result))
            print_data_table(std::get<std::unique_ptr<DataTable>>(result.result).get());
        else
            std::cout << "Rows affected: " << std::get<int>(result.result) << std::endl;
    }

    std::cout << "Exiting deltabase.\n";
}

