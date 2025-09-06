#include "include/data_object.hpp"
#include "../misc/include/utils.hpp"
#include <cstring>
#include <cstdlib>
#include <memory>

extern "C" {
#include "../core/include/binary_io.h"
}

namespace catalog {
    // CppDataToken implementation
    CppDataToken::CppDataToken() : size(0), bytes(nullptr), type(DT_NULL) {}

    CppDataToken::CppDataToken(size_t size, char* bytes, DataType type) 
        : size(size), type(type) {
        if (size > 0 && bytes) {
            this->bytes = std::make_unique<char[]>(size);
            memcpy(this->bytes.get(), bytes, size);
        }
    }

    CppDataToken::CppDataToken(const DataToken& token) 
        : size(token.size), type(token.type) {
        if (size > 0 && token.bytes) {
            bytes = std::make_unique<char[]>(size);
            memcpy(bytes.get(), token.bytes, size);
        }
    }

    CppDataToken::CppDataToken(const CppDataToken& other) 
        : size(other.size), type(other.type) {
        if (size > 0 && other.bytes) {
            bytes = std::make_unique<char[]>(size);
            memcpy(bytes.get(), other.bytes.get(), size);
        }
    }

    auto CppDataToken::operator=(const CppDataToken& other) -> CppDataToken& {
        if (this != &other) {
            size = other.size;
            type = other.type;
            if (size > 0 && other.bytes) {
                bytes = std::make_unique<char[]>(size);
                memcpy(bytes.get(), other.bytes.get(), size);
            } else {
                bytes.reset();
            }
        }
        return *this;
    }

    auto CppDataToken::to_c() const -> DataToken {
        DataToken result;
        result.size = size;
        result.type = type;
        if (size > 0 && bytes) {
            result.bytes = (char*)malloc(size);
            memcpy(result.bytes, bytes.get(), size);
        } else {
            result.bytes = nullptr;
        }
        return result;
    }

    // CppDataRow implementation
    CppDataRow::CppDataRow() : row_id(0), flags(RF_NONE) {}

    CppDataRow::CppDataRow(uint64_t row_id, std::initializer_list<CppDataToken> tokens, DataRowFlags flags)
        : row_id(row_id), flags(flags), tokens(tokens) {}

    CppDataRow::CppDataRow(const DataRow& row) 
        : row_id(row.row_id), flags(row.flags) {
        tokens.reserve(row.count);
        for (uint64_t i = 0; i < row.count; ++i) {
            tokens.emplace_back(row.tokens[i]);
        }
    }

    auto CppDataRow::to_c() const -> DataRow {
        DataRow result;
        result.row_id = row_id;
        result.flags = flags;
        result.count = tokens.size();
        
        if (result.count > 0) {
            result.tokens = (DataToken*)malloc(result.count * sizeof(DataToken));
            for (size_t i = 0; i < result.count; ++i) {
                result.tokens[i] = tokens[i].to_c();
            }
        } else {
            result.tokens = nullptr;
        }
        
        return result;
    }

    void CppDataRow::cleanup_c(DataRow& row) {
        if (row.tokens) {
            for (uint64_t i = 0; i < row.count; ++i) {
                free(row.tokens[i].bytes);
            }
            free(row.tokens);
        }
    }

    // CppDataTable implementation
    CppDataTable::CppDataTable() {}

    CppDataTable::CppDataTable(const CppMetaTable& schema) : schema_(schema) {}

    CppDataTable::CppDataTable(const DataTable& table) 
        : schema_(CppMetaTable::from_c(table.schema)), rows_(parse_rows(table)) {}

    DataTable CppDataTable::to_c() const {
        DataTable result;
        result.schema = schema_.to_c();
        result.rows_count = rows_.size();
        
        if (result.rows_count > 0) {
            result.rows = (DataRow*)malloc(result.rows_count * sizeof(DataRow));
            for (size_t i = 0; i < result.rows_count; ++i) {
                result.rows[i] = rows_[i].to_c();
            }
        } else {
            result.rows = nullptr;
        }
        
        return result;
    }

    const std::vector<CppDataRow>& CppDataTable::get_rows() {
        return rows_;
    }

    void CppDataTable::add_row(const CppDataRow& row) {
        rows_.push_back(row);
    }

    auto CppDataTable::parse_rows(const DataTable& table) -> std::vector<CppDataRow> {
        std::vector<CppDataRow> result;
        result.reserve(table.rows_count);
        
        for (size_t i = 0; i < table.rows_count; ++i) {
            result.emplace_back(table.rows[i]);
        }
        
        return result;
    }

    // CppDataFilterCondition implementation
    CppDataFilterCondition CppDataFilterCondition::from_c(const DataFilterCondition& dfc) {
        CppDataFilterCondition result;
        result.column_id = make_uuid_str(dfc.column_id);
        result.operation = dfc.op;
        result.type = dfc.type;
        
        if (dfc.value) {
            uint64_t static_size = dtype_size(dfc.type);
            size_t value_size = 0;
            
            if (static_size > 0) {
                value_size = static_size;
            } else if (dfc.type == DT_STRING) {
                value_size = strlen(static_cast<const char*>(dfc.value)) + 1;
            }
            
            if (value_size > 0) {
                void* value_copy = malloc(value_size);
                memcpy(value_copy, dfc.value, value_size);
                result.value = unique_void_ptr(value_copy);
            }
        }
        
        return result;
    }

    DataFilterCondition CppDataFilterCondition::to_c() const {
        DataFilterCondition result;
        parse_uuid_str(column_id, result.column_id);
        result.op = operation;
        result.type = type;
        
        if (value) {
            uint64_t static_size = dtype_size(type);
            size_t value_size = 0;
            
            if (static_size > 0) {
                value_size = static_size;
            } else if (type == DT_STRING) {
                value_size = strlen(static_cast<const char*>(value.get())) + 1;
            }
            
            if (value_size > 0) {
                result.value = malloc(value_size);
                memcpy(result.value, value.get(), value_size);
            } else {
                result.value = nullptr;
            }
        } else {
            result.value = nullptr;
        }
        
        return result;
    }

    void CppDataFilterCondition::cleanup_c(DataFilterCondition& dfc) {
        free(dfc.value);
    }

    // CppDataFilterNode implementation
    CppDataFilterNode CppDataFilterNode::from_c(const DataFilterNode& dfn) {
        CppDataFilterNode result;
        result.op = dfn.op;
        
        if (dfn.left) {
            result.left = std::make_shared<CppDataFilter>(CppDataFilter::from_c(*dfn.left));
        }
        
        if (dfn.right) {
            result.right = std::make_shared<CppDataFilter>(CppDataFilter::from_c(*dfn.right));
        }
        
        return result;
    }

    DataFilterNode CppDataFilterNode::to_c() const {
        DataFilterNode result;
        result.op = op;
        
        if (left) {
            result.left = (DataFilter*)malloc(sizeof(DataFilter));
            *result.left = left->to_c();
        } else {
            result.left = nullptr;
        }
        
        if (right) {
            result.right = (DataFilter*)malloc(sizeof(DataFilter));
            *result.right = right->to_c();
        } else {
            result.right = nullptr;
        }
        
        return result;
    }

    void CppDataFilterNode::cleanup_c(DataFilterNode& node) {
        if (node.left) {
            CppDataFilter::cleanup_c(*node.left);
            free(node.left);
        }
        if (node.right) {
            CppDataFilter::cleanup_c(*node.right);
            free(node.right);
        }
    }

    // CppDataFilter implementation
    CppDataFilter CppDataFilter::from_c(const DataFilter& df) {
        CppDataFilter result;
        
        if (df.is_node) {
            result.value = CppDataFilterNode::from_c(df.data.node);
        } else {
            result.value = CppDataFilterCondition::from_c(df.data.condition);
        }
        
        return result;
    }

    DataFilter CppDataFilter::to_c() const {
        DataFilter result;
        
        if (std::holds_alternative<CppDataFilterNode>(value)) {
            result.is_node = true;
            result.data.node = std::get<CppDataFilterNode>(value).to_c();
        } else {
            result.is_node = false;
            result.data.condition = std::get<CppDataFilterCondition>(value).to_c();
        }
        
        return result;
    }

    void CppDataFilter::cleanup_c(DataFilter& df) {
        if (df.is_node) {
            CppDataFilterNode::cleanup_c(df.data.node);
        } else {
            CppDataFilterCondition::cleanup_c(df.data.condition);
        }
    }

    // CppDataRowUpdate implementation
    CppDataRowUpdate::CppDataRowUpdate(const CppMetaTable& table) : table_schema_(table) {}

    // Helper function to determine data size by type
    size_t get_data_type_size(DataType type, const void* data) {
        uint64_t static_size = dtype_size(type);
        if (static_size > 0) {
            return static_size;
        }
        
        // Handle dynamic size types (strings)
        if (type == DT_STRING && data) {
            return strlen(static_cast<const char*>(data)) + 1;
        }
        
        return 0;
    }

    CppDataRowUpdate CppDataRowUpdate::from_c(const DataRowUpdate& update, const CppMetaTable& table) {
        CppDataRowUpdate result(table);
        result.assignments.reserve(update.count);
        
        for (size_t i = 0; i < update.count; ++i) {
            // Find the column by UUID
            std::string column_uuid = make_uuid_str(update.column_indices[i]);
            
            // Find corresponding column in table schema
            CppMetaColumn column;
            bool found = false;
            for (const auto& col : table.columns) {
                if (col.id == column_uuid) {
                    column = col;
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                throw std::runtime_error("Column not found in table schema");
            }
            
            // Copy the value with correct size based on data type
            unique_void_ptr value_copy;
            if (update.values[i]) {
                size_t value_size = get_data_type_size(column.data_type, update.values[i]);
                if (value_size > 0) {
                    void* copy = malloc(value_size);
                    memcpy(copy, update.values[i], value_size);
                    value_copy = unique_void_ptr(copy);
                }
            }
            
            result.assignments.emplace_back(std::move(column), std::move(value_copy));
        }
        
        return result;
    }

    DataRowUpdate CppDataRowUpdate::to_c() const {
        DataRowUpdate result;
        result.count = assignments.size();
        
        if (result.count > 0) {
            result.column_indices = (uuid_t*)malloc(result.count * sizeof(uuid_t));
            result.values = (void**)malloc(result.count * sizeof(void*));
            
            for (size_t i = 0; i < result.count; ++i) {
                parse_uuid_str(assignments[i].first.id, result.column_indices[i]);
                
                if (assignments[i].second) {
                    size_t value_size = get_data_type_size(assignments[i].first.data_type, assignments[i].second.get());
                    if (value_size > 0) {
                        result.values[i] = malloc(value_size);
                        memcpy(result.values[i], assignments[i].second.get(), value_size);
                    } else {
                        result.values[i] = nullptr;
                    }
                } else {
                    result.values[i] = nullptr;
                }
            }
        } else {
            result.column_indices = nullptr;
            result.values = nullptr;
        }
        
        return result;
    }

    void
    CppDataRowUpdate::cleanup_c(DataRowUpdate& update) {
        if (update.column_indices) {
            free(update.column_indices);
        }
        if (update.values) {
            for (size_t i = 0; i < update.count; ++i) {
                free(update.values[i]);
            }
            free(update.values);
        }
    }

    // Helper function to create CppDataRowUpdate from DataRowUpdate and table
    CppDataRowUpdate
    create_cpp_data_row_update(const DataRowUpdate& c_update, const CppMetaTable& table) {
        return CppDataRowUpdate::from_c(c_update, table);
    }
}