#include "include/data_object.hpp"

namespace catalog {

    // CppDataToken implementations
    CppDataToken::CppDataToken() : size(0), bytes(nullptr), type(DT_UNDEFINED) {
    }

    CppDataToken::CppDataToken(size_t size, char* bytes, DataType type) : size(size), type(type) {
        if (bytes && size > 0) {
            this->bytes = std::make_unique<char[]>(size);
            std::memcpy(this->bytes.get(), bytes, size);
        }
    }

    CppDataToken::CppDataToken(const DataToken& token) : size(token.size), type(token.type) {
        if (token.bytes && token.size > 0) {
            this->bytes = std::make_unique<char[]>(size);
            std::memcpy(bytes.get(), token.bytes, size);
        }
    }

    CppDataToken::CppDataToken(const CppDataToken& other) : size(other.size), type(other.type) {
        if (other.bytes && other.size > 0) {
            this->bytes = std::make_unique<char[]>(size);
            std::memcpy(bytes.get(), other.bytes.get(), size);
        }
    }

    auto CppDataToken::operator=(const CppDataToken& other) -> CppDataToken& {
        if (this != &other) {
            size = other.size;
            type = other.type;
            if (other.bytes && other.size > 0) {
                bytes = std::make_unique<char[]>(size);
                std::memcpy(bytes.get(), other.bytes.get(), size);
            } else {
                bytes.reset();
            }
        }
        return *this;
    }

    auto CppDataToken::to_data_token() const -> DataToken {
        DataToken token;
        token.size = size;
        token.type = type;
        if (bytes && size > 0) {
            token.bytes = static_cast<char*>(std::malloc(size));
            std::memcpy(token.bytes, bytes.get(), size);
        } else {
            token.bytes = nullptr;
        }
        return token;
    }

    // CppDataRow implementations
    CppDataRow::CppDataRow() : row_id(0), flags(static_cast<DataRowFlags>(0)) {
    }

    CppDataRow::CppDataRow(uint64_t row_id,
                           std::initializer_list<CppDataToken> tokens,
                           DataRowFlags flags)
        : row_id(row_id), flags(flags), tokens(tokens) {
    }

    CppDataRow::CppDataRow(const DataRow& row) : row_id(row.row_id), flags(row.flags) {
        tokens.reserve(row.count);
        for (uint64_t i = 0; i < row.count; i++) {
            tokens.emplace_back(row.tokens[i]);
        }
    }

    auto CppDataRow::to_data_row() const -> DataRow {
        DataRow row;
        row.row_id = row_id;
        row.flags = flags;
        row.count = tokens.size();

        if (row.count > 0) {
            row.tokens = static_cast<DataToken*>(std::malloc(tokens.size() * sizeof(DataToken)));
            for (uint64_t i = 0; i < row.count; i++) {
                row.tokens[i] = tokens[i].to_data_token();
            }
        } else {
            row.tokens = nullptr;
        }
        
        return row;
    }

    // CppDataTable implementations
    CppDataTable::CppDataTable() = default;

    CppDataTable::CppDataTable(const CppMetaTable& schema) : schema_(schema) {
    }

    CppDataTable::CppDataTable(const DataTable& other) 
        : schema_(other.schema), rows_(this->parse_rows(other)) {
    }

    auto CppDataTable::to_data_table() const -> DataTable {
        DataTable table;
        table.schema = schema_.create_meta_table();
        table.rows_count = rows_.size();
        
        if (rows_.size() > 0) {
            table.rows = static_cast<DataRow*>(std::malloc(rows_.size() * sizeof(DataRow)));
            for (size_t i = 0; i < rows_.size(); i++) {
                table.rows[i] = rows_[i].to_data_row();
            }
        } else {
            table.rows = nullptr;
        }
        
        return table;
    }

    auto CppDataTable::get_rows() -> const std::vector<CppDataRow>& {
        return rows_;
    }

    void CppDataTable::add_row(const CppDataRow& row) {
        rows_.push_back(row);
    }

    auto CppDataTable::parse_rows(const DataTable& table) -> std::vector<CppDataRow> {
        std::vector<CppDataRow> result;
        result.reserve(table.rows_count);
        
        for (size_t i = 0; i < table.rows_count; i++) {
            result.emplace_back(table.rows[i]);
        }
        
        return result;
    }


} // namespace catalog