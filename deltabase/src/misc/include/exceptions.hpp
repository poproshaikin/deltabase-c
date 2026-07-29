#ifndef MISC_EXCEPTIONS_H
#define MISC_EXCEPTIONS_H

#include <stdexcept>
#include <string>

class EngineException : public std::runtime_error
{
public:
    enum class Code
    {
        GENERIC,

        // Object existence
        DB_NOT_ATTACHED,
        DB_NOT_EXISTS,
        DB_EXISTS,
        SCHEMA_NOT_EXISTS,
        SCHEMA_EXISTS,
        TABLE_NOT_EXISTS,
        TABLE_EXISTS,
        COLUMN_NOT_EXISTS,
        COLUMN_EXISTS,
        INDEX_NOT_EXISTS,
        INDEX_EXISTS,

        // Constraint violations
        UNIQUE_VIOLATION,
        NOT_NULL_VIOLATION,
        CHECK_VIOLATION,
        FOREIGN_KEY_VIOLATION,

        // SQL syntax / parsing
        SYNTAX_ERROR,
        UNSUPPORTED_STATEMENT,
        UNEXPECTED_TOKEN,

        // Semantic / query errors
        TYPE_MISMATCH,
        COLUMN_COUNT_MISMATCH,
        AMBIGUOUS_COLUMN,
        INVALID_COMPARISON,
        NULLABLE_PK,
        MULTIPLE_PK,
        INVALID_AUTOINCREMENT_TYPE,
        MULTIPLE_BEGIN,
        NO_ACTIVE_TXN,
        REF_TABLE_NOT_EXISTS,
        REF_COLUMN_NOT_EXISTS,
        REF_COLUMN_TYPE_MISMATCH,
        REF_COLUMN_NOT_UNIQUE,
        REF_COLUMN_NOT_NULL
    };

    explicit EngineException(const std::string& msg, Code code)
        : std::runtime_error(msg), code_(code)
    {
    }

    Code code() const { return code_; }

private:
    Code code_;
};

#endif
