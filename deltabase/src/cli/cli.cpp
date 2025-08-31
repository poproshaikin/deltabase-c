#include "include/cli.hpp"
#include "../misc/include/utils.hpp"
#include "../sql/include/lexer.hpp"
#include "../sql/include/parser.hpp"
#include <iomanip>
#include <iostream>
#include <memory>

using namespace cli;
using namespace engine;


// Forward declarations for print functions
void
print_ast_node(const std::unique_ptr<sql::AstNode>& node, int indent);
void
print_ast_node(const sql::AstNode& node, int indent);
void
print_sql_token(const sql::SqlToken& token, int indent);
void
print_column_definition(const sql::ColumnDefinition& col, int indent);
void
print_table_identifier(const sql::TableIdentifier& table, int indent);

void
print_sql_token(const sql::SqlToken& token, int indent) {
    std::string indent_str(indent, ' ');
    std::cout << indent_str << "SqlToken: " << token.to_string() << "\n";
}

void
print_ast_node(const sql::AstNode& node, int indent) {
    std::string indent_str(indent, ' ');
    std::cout << indent_str << "Node type: " << static_cast<int>(node.type) << "\n";

    std::visit(
        [&](auto&& value) {
            using T = std::decay_t<decltype(value)>;

            if constexpr (std::is_same_v<T, sql::SqlToken>) {
                print_sql_token(value, indent + 2);
            } else if constexpr (std::is_same_v<T, sql::BinaryExpr>) {
                std::cout << indent_str << "  BinaryExpr:\n";
                std::cout << indent_str << "    Operator: " << static_cast<int>(value.op) << "\n";
                std::cout << indent_str << "    Left:\n";
                print_ast_node(*value.left, indent + 6);
                std::cout << indent_str << "    Right:\n";
                print_ast_node(*value.right, indent + 6);
            } else if constexpr (std::is_same_v<T, sql::SelectStatement>) {
                std::cout << indent_str << "  SelectStatement:\n";
                std::cout << indent_str << "    Table:\n";
                print_table_identifier(value.table, indent + 6);
                std::cout << indent_str << "    Columns:\n";
                for (const auto& col : value.columns) {
                    print_sql_token(col, indent + 6);
                }
                if (value.where) {
                    std::cout << indent_str << "    Where:\n";
                    print_ast_node(*value.where, indent + 6);
                }
            } else if constexpr (std::is_same_v<T, sql::UpdateStatement>) {
                std::cout << indent_str << "  UpdateStatement:\n";
                std::cout << indent_str << "    Table:\n";
                print_table_identifier(value.table, indent + 6);

                std::cout << indent_str << "    Assignments:\n";
                for (const auto& assign : value.assignments) {
                    print_ast_node(assign, indent + 6);
                }

                std::cout << indent_str << "    Where:\n";
                if (value.where) {
                    print_ast_node(*value.where, indent + 6);
                } else {
                    std::cout << indent_str << "      (none)\n";
                }

            } else if constexpr (std::is_same_v<T, sql::DeleteStatement>) {
                std::cout << indent_str << "  DeleteStatement:\n";
                std::cout << indent_str << "    Table:\n";
                print_table_identifier(value.table, indent + 6);

                std::cout << indent_str << "    Where:\n";
                if (value.where) {
                    print_ast_node(*value.where, indent + 6);
                } else {
                    std::cout << indent_str << "      (none)\n";
                }
            } else if constexpr (std::is_same_v<T, sql::InsertStatement>) {
                std::cout << indent_str << "  InsertStatement:\n";
                std::cout << indent_str << "    Table:\n";
                print_table_identifier(value.table, indent + 6);
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
                print_table_identifier(value.table, indent + 6);
                std::cout << indent_str << "    Columns:\n";
                for (const auto& col : value.columns) {
                    print_column_definition(col, indent + 6);
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
        },
        node.value);
}

void
print_ast_node(const std::unique_ptr<sql::AstNode>& node, int indent) {
    if (!node) {
        std::cout << std::string(indent, ' ') << "(null)\n";
        return;
    }
    print_ast_node(*node, indent);
}

std::string
token_to_string(const DataToken* token) {
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
        return std::string(1, *token->bytes); // 1 символ
    }
    case DT_STRING: {
        return std::string(token->bytes, token->size);
    }
    default:
        return "<unknown>";
    }
}

void
print_data_table(const DataTable* table) {
    if (!table || !table->scheme) {
        std::cerr << "Invalid DataTable\n";
        return;
    }

    const MetaTable* schema = table->scheme;

    for (size_t i = 0; i < schema->columns_count; ++i) {
        std::cout << std::left << std::setw(15) << schema->columns[i].name;
    }
    std::cout << "\n";

    for (size_t i = 0; i < schema->columns_count; ++i) {
        std::cout << std::setw(15) << std::setfill('-') << "";
    }
    std::cout << std::setfill(' ') << "\n";

    for (size_t r = 0; r < table->rows_count; ++r) {
        const DataRow& row = table->rows[r];
        for (size_t c = 0; c < schema->columns_count; ++c) {
            const DataToken& token = row.tokens[c];
            std::cout << std::left << std::setw(15) << ::token_to_string(&token);
        }
        std::cout << "\n";
    }

    std::cout << "Rows: " << table->rows_count << std::endl;
}

SqlCli::SqlCli() {
    this->add_cmd_handler("q", [](std::string arg) {
        std::cout << "Exiting deltabase." << std::endl;
        std::exit(0);
    });
    this->add_cmd_handler("c", [this](std::string arg) {
        this->engine = std::make_unique<DltEngine>(arg);
        std::cout << "Connected successfuly to the '" << arg << "'" << std::endl;
    });
}

void SqlCli::run_cmd(const std::string& input) {
        if (input[0] == '\\') {
            std::string cmd = input.substr(1);
            auto splitted = split(cmd, ' ', 1);

            std::string command = splitted[0];
            std::string argument = (splitted.size() > 1) ? splitted[1] : "";

            if (this->handlers.count(command) != 0) {
                handlers[command](argument);
            }
        } else {
            if (engine == nullptr) {
                std::cout << "You are not connected to a database. Connect first using \\c"
                          << std::endl;
            } else {
                try {
                    const ExecutionResult& result = engine->run_query(input);

                    std::cout << "Execution time: " << result.execution_time_ns / 1000000.0 << " ms"
                              << std::endl;

                    if (std::holds_alternative<std::unique_ptr<DataTable>>(result.result))
                        print_data_table(std::get<std::unique_ptr<DataTable>>(result.result).get());
                    else
                        std::cout << "Rows affected: " << std::get<int>(result.result) << std::endl;
                } catch (std::runtime_error e) {
                    std::cout << "Failed to execute query: " << e.what() << std::endl;
                }
            }
        }
}

void
SqlCli::run_query_console() {
    std::string input;
    this->engine = std::make_unique<DltEngine>();
    std::cout << "Welcome to deltabase! Type '\\q' to quit." << std::endl;

    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);
        run_cmd(input);
    }
}

void
SqlCli::add_cmd_handler(std::string cmd, std::function<void(std::string arg)> func) {
    this->handlers[cmd] = func;
}

void
print_table_identifier(const sql::TableIdentifier& table, int indent) {
    std::string indent_str(indent, ' ');
    if (table.schema_name) {
        std::cout << indent_str << "Schema: " << table.schema_name->value << "\n";
    }
    std::cout << indent_str << "Table: " << table.table_name.value << "\n";
}

void
print_column_definition(const sql::ColumnDefinition& col, int indent) {
    std::string indent_str(indent, ' ');
    std::cout << indent_str << "ColumnDefinition:\n";
    std::cout << indent_str << "  Name: " << col.name.value << "\n";
    std::cout << indent_str << "  Type: " << col.type.value << "\n";
    if (!col.constraints.empty()) {
        std::cout << indent_str << "  Constraints:\n";
        for (const auto& constraint : col.constraints) {
            std::cout << indent_str << "    " << constraint.value << "\n";
        }
    }
}

// CREATE TABLE users(id INTEGER, name STRING, age REAL)