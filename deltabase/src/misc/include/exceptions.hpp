#ifndef MISC_EXCEPTIONS_H
#define MISC_EXCEPTIONS_H

#include <stdexcept>
#include <string>

class InvalidStatementSyntax : public std::runtime_error {
  public:
    InvalidStatementSyntax(const std::string& msg = "Invalid statement syntax")
        : std::runtime_error(msg) {
    }
};

class TableExists : public std::runtime_error {
  public:
    TableExists(const std::string& table_name)
        : std::runtime_error("Table '" + table_name + "' already exists") {
    }
};

class TableDoesntExist : public std::runtime_error {
  public:
    TableDoesntExist(const std::string& table_name)
        : std::runtime_error("Table '" + table_name + "' doesnt exist") {
    }
};

class ColumnDoesntExists : public std::runtime_error {
  public:
    ColumnDoesntExists(const std::string& col_name)
        : std::runtime_error("Column '" + col_name + "' doesn't exists") {
    }
};

class DbDoesntExists : public std::runtime_error {
  public:
    DbDoesntExists(const std::string& db_name)
        : std::runtime_error("Database '" + db_name + "' doesn't exists") {
    }
};

class DbExists : public std::runtime_error {
  public:
    DbExists(const std::string& db_name)
        : std::runtime_error("Database '" + db_name + "' already exists") {
    }
};

#endif