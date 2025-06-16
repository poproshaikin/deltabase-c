#include "include/cli.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>

extern "C" {
    #include "../core/include/core.h"
}

namespace cli {
    // void print_ast_node(const std::unique_ptr<sql::AstNode>& node, int indent) {
    //     if (!node) {
    //         std::cout << std::string(indent, ' ') << "(null)\n";
    //         return;
    //     }
    //
    //     std::string indent_str(indent, ' ');
    //     std::cout << indent_str << "Node type: " << static_cast<int>(node->type) << "\n";
    //
    //     std::visit([&](auto&& value) {
    //             using T = std::decay_t<decltype(value)>;
    //
    //             if constexpr (std::is_same_v<T, sql::SqlToken>) {
    //             std::cout << indent_str << "  SqlToken: " << value.to_string(indent + 6) << "\n";
    //             } else if constexpr (std::is_same_v<T, sql::BinaryExpr>) {
    //             std::cout << indent_str << "  BinaryExpr:\n";
    //             std::cout << indent_str << "    Operator: " << static_cast<int>(value.op) << "\n";
    //             std::cout << indent_str << "    Left:\n";
    //             print_ast_node(value.left, indent + 6);
    //             std::cout << indent_str << "    Right:\n";
    //             print_ast_node(value.right, indent + 6);
    //             } else if constexpr (std::is_same_v<T, sql::SelectStatement>) {
    //             std::cout << indent_str << "  SelectStatement:\n";
    //             std::cout << indent_str << "    Table:\n";
    //             print_ast_node(value.table, indent + 6);
    //             std::cout << indent_str << "    Columns:\n";
    //             for (const auto& col : value.columns) {
    //             print_ast_node(col, indent + 6);
    //             }
    //             if (value.where) {
    //                 std::cout << indent_str << "    Where:\n";
    //                 print_ast_node(value.where, indent + 6);
    //             }
    //             } else if constexpr (std::is_same_v<T, sql::UpdateStatement>) {
    //                 std::cout << indent_str << "  UpdateStatement:\n";
    //                 std::cout << indent_str << "    Table:\n";
    //                 print_ast_node(value.table, indent + 6);
    //
    //                 std::cout << indent_str << "    Assignments:\n";
    //                 for (const auto& assign : value.assignments) {
    //                     print_ast_node(assign, indent + 6);
    //                 }
    //
    //                 std::cout << indent_str << "    Where:\n";
    //                 if (value.where) {
    //                     print_ast_node(value.where, indent + 6);
    //                 } else {
    //                     std::cout << indent_str << "      (none)\n";
    //                 }
    //
    //             } else if constexpr (std::is_same_v<T, sql::DeleteStatement>) {
    //                 std::cout << indent_str << "  DeleteStatement:\n";
    //                 std::cout << indent_str << "    Table:\n";
    //                 print_ast_node(value.table, indent + 6);
    //
    //                 std::cout << indent_str << "    Where:\n";
    //                 if (value.where) {
    //                     print_ast_node(value.where, indent + 6);
    //                 } else {
    //                     std::cout << indent_str << "      (none)\n";
    //                 }
    //             }
    //             else if constexpr (std::is_same_v<T, sql::InsertStatement>) {
    //                 std::cout << indent_str << "  InsertStatement:\n";
    //                 std::cout << indent_str << "    Table:\n";
    //                 print_ast_node(value.table, indent + 6);
    //                 std::cout << indent_str << "    Columns:\n";
    //                 for (const auto& col : value.columns) {
    //                     print_ast_node(col, indent + 6);
    //                 }
    //                 std::cout << indent_str << "    Values:\n";
    //                 for (const auto& val : value.values) {
    //                     print_ast_node(val, indent + 6);
    //                 }
    //             } else {
    //                 std::cout << indent_str << "  (unknown node type)\n";
    //             }
    //     }, node->value);
    // }
    //
    // std::string token_to_string(const DataToken* token) {
    //     if (!token || token->type == DT_NULL) {
    //         return "NULL";
    //     }
    //
    //     switch (token->type) {
    //         case DT_INTEGER: {
    //                              int32_t value;
    //                              memcpy(&value, token->bytes, sizeof(value));
    //                              return std::to_string(value);
    //                          }
    //         case DT_REAL: {
    //                           double value;
    //                           memcpy(&value, token->bytes, sizeof(value));
    //                           return std::to_string(value);
    //                       }
    //         case DT_BOOL: {
    //                           bool value;
    //                           memcpy(&value, token->bytes, sizeof(value));
    //                           return value ? "true" : "false";
    //                       }
    //         case DT_CHAR: {
    //                           return std::string(1, *token->bytes);  // 1 символ
    //                       }
    //         case DT_STRING: {
    //                             return std::string(token->bytes, token->size);
    //                         }
    //         default:
    //                         return "<unknown>";
    //     }
    // }
    //
    // void print_data_table(const DataTable* table) {
    //     if (!table || !table->scheme) {
    //         std::cerr << "Invalid DataTable\n";
    //         return;
    //     }
    //
    //     const MetaTable* schema = table->scheme;
    //
    //     for (size_t i = 0; i < schema->columns_count; ++i) {
    //         std::cout << std::left << std::setw(15) << schema->columns[i]->name;
    //     }
    //     std::cout << "\n";
    //
    //     for (size_t i = 0; i < schema->columns_count; ++i) {
    //         std::cout << std::setw(15) << std::setfill('-') << "";
    //     }
    //     std::cout << std::setfill(' ') << "\n";
    //
    //     for (size_t r = 0; r < table->rows_count; ++r) {
    //         const DataRow* row = table->rows[r];
    //         for (size_t c = 0; c < schema->columns_count; ++c) {
    //             const DataToken* token = row->tokens[c];
    //             std::cout << std::left << std::setw(15) << token_to_string(token);
    //         }
    //         std::cout << "\n";
    //     }
    // }
    //
    // void SqlCli::run_query_console() {
    //     std::string input;
    //     std::cout << "Welcome to deltabase! Type 'exit' to quit.\n";
    //
    //     while (true) {
    //         std::cout << "> ";
    //         if (!std::getline(std::cin, input)) {
    //             break;
    //         }
    //         if (input == "exit") {
    //             break;
    //         }
    //         process_input(input);
    //     }
    //
    //     std::cout << "Exiting deltabase.\n";
    // }
    //
    // void SqlCli::process_input(const std::string& input) {
    //     std::string db_name = "testdb";
    //
    //     auto start = std::chrono::high_resolution_clock::now();
    //
    //     sql::SqlTokenizer tokenizer;
    //     std::vector<sql::SqlToken> tokens = tokenizer.tokenize(input);
    //
    //     sql::SqlParser parser(tokens);
    //     std::unique_ptr<sql::AstNode> node = parser.parse();
    //
    //     exe::SemanticAnalyzer analyzer(db_name);
    //     analyzer.analyze(node.get());
    //
    //     exe::QueryExecutor executor(db_name);
    //     const auto result = executor.execute(*node);
    //
    //     auto end = std::chrono::high_resolution_clock::now();
    //     auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    //     std::cout << "Execution time: " << duration_ms << " ms" << std::endl;
    //
    //     if (std::holds_alternative<std::unique_ptr<DataTable>>(result))
    //         print_data_table(std::get<std::unique_ptr<DataTable>>(result).get());
    //     else 
    //         std::cout << "Rows affected: " << std::get<int>(result) << std::endl;
    // }
}
