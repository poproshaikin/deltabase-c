#ifndef MISC_EXCEPTIONS_H
#define MISC_EXCEPTIONS_H

#include <string>
#include <stdexcept>

class InvalidStatementSyntax : public std::runtime_error {
    public:
        InvalidStatementSyntax(const std::string& msg = "Invalid statement syntax") : std::runtime_error(msg) { }
};

class TableExists : public std::runtime_error {
    public:
        TableExists(const std::string& table_name) : 
            std::runtime_error("Table '" + table_name + "' already exists") { }
};

class ColumnDoesntExists : public std::runtime_error {
    public:
        ColumnDoesntExists(const std::string& col_name) :
            std::runtime_error("Column '" + col_name + "' doesn't exists") { }
};

#endif