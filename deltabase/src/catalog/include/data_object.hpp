#pragma once

#include "meta_schema.hpp"
#include <cstring>
#include <memory>

extern "C" {
#include "../../core/include/data.h"
}

namespace catalog {

    struct CppDataToken {
        size_t size;
        std::unique_ptr<char[]> bytes;
        DataType type;

        CppDataToken();
        CppDataToken(size_t size, char* bytes, DataType type);
        CppDataToken(const DataToken& token);
        
        // Copy constructor
        CppDataToken(const CppDataToken& other);
        
        // Move constructor
        CppDataToken(CppDataToken&& other) noexcept = default;
        
        // Copy assignment
        CppDataToken& operator=(const CppDataToken& other);
        
        // Move assignment
        CppDataToken& operator=(CppDataToken&& other) noexcept = default;
        
        // Convert back to C DataToken
        DataToken to_data_token() const;
    };

    struct CppDataRow {
        uint64_t row_id;
        DataRowFlags flags;
        std::vector<CppDataToken> tokens;

        CppDataRow();
        CppDataRow(uint64_t row_id,
                   std::initializer_list<CppDataToken> tokens,
                   DataRowFlags flags = RF_NONE);
        CppDataRow(const DataRow& row);
        
        ~CppDataRow() = default;
        
        DataRow to_data_row() const;
    };

    struct CppDataTable {
        CppDataTable();
        CppDataTable(const CppMetaTable& schema);
        CppDataTable(const DataTable& table);
        
        ~CppDataTable() = default;

        DataTable to_data_table() const;
        
        const std::vector<CppDataRow>& get_rows();
        
        void add_row(const CppDataRow& row);

    private:
        CppMetaTable schema;
        std::vector<CppDataRow> rows;

        std::vector<CppDataRow> parse_rows(const DataTable& table);
    };
}