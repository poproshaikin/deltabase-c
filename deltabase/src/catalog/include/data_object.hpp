#pragma once

#include "meta_object.hpp"
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
        auto operator=(const CppDataToken& other) -> CppDataToken&;
        
        // Move assignment
        auto operator=(CppDataToken&& other) noexcept -> CppDataToken& = default;
        
        // Convert back to C DataToken
        [[nodiscard]] auto to_data_token() const -> DataToken;
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
        
        [[nodiscard]] auto to_data_row() const -> DataRow;
    };

    struct CppDataTable {
        CppDataTable();
        CppDataTable(const CppMetaTable& schema);
        CppDataTable(const DataTable& table);
        
        ~CppDataTable() = default;

        [[nodiscard]] auto to_data_table() const -> DataTable;
        
        auto get_rows() -> const std::vector<CppDataRow>&;
        
        void add_row(const CppDataRow& row);

    private:
        CppMetaTable schema_;
        std::vector<CppDataRow> rows_;

        auto parse_rows(const DataTable& table) -> std::vector<CppDataRow>;
    };
}