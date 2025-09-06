#pragma once

#include "../../catalog/include/meta_object.hpp"

#include <cstddef>
#include <memory>
#include <vector>
#include <variant>

extern "C" {
#include "../../core/include/meta.h"
#include "../../core/include/data.h"
#include "../../core/include/misc.h"
}

struct FreeDeleter {
    void operator()(void* ptr) {
        std::free(ptr);
    }
};

using unique_void_ptr = std::unique_ptr<void, FreeDeleter>;

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
        [[nodiscard]] auto to_c() const -> DataToken;
    };

    struct CppDataRow {
        uint64_t row_id;
        DataRowFlags flags;
        std::vector<CppDataToken> tokens;

        CppDataRow();
        CppDataRow(
            uint64_t row_id,
            std::initializer_list<CppDataToken> tokens,
            DataRowFlags flags = RF_NONE
        );
        CppDataRow(const DataRow& row);
        
        ~CppDataRow() = default;
        [[nodiscard]] auto
        to_c() const -> DataRow;

        static void
        cleanup_c(DataRow& row);
    };

    struct CppDataTable {
        CppDataTable();
        CppDataTable(const CppMetaTable& schema);
        CppDataTable(const DataTable& table);
        
        ~CppDataTable() = default;

        [[nodiscard]] DataTable
        to_c() const;

        const std::vector<CppDataRow>&
        get_rows();

        void add_row(const CppDataRow& row);

    private:
        CppMetaTable schema_;
        std::vector<CppDataRow> rows_;

        auto parse_rows(const DataTable& table) -> std::vector<CppDataRow>;
    };

    struct CppDataFilterCondition {
        std::string column_id;
        FilterOp operation;
        DataType type;
        unique_void_ptr value;

        static CppDataFilterCondition
        from_c(const DataFilterCondition& dfc);
        [[nodiscard]] DataFilterCondition
        to_c() const;

        static void
        cleanup_c(DataFilterCondition& dfc);
    };

    struct CppDataFilter;

    struct CppDataFilterNode {
        std::shared_ptr<CppDataFilter> left;
        LogicOp op;
        std::shared_ptr<CppDataFilter> right;

        static CppDataFilterNode
        from_c(const DataFilterNode& dfn);
        [[nodiscard]] DataFilterNode
        to_c() const;

        static void
        cleanup_c(DataFilterNode& node);
    };

    struct CppDataFilter {
        std::variant<CppDataFilterCondition, CppDataFilterNode> value;

        static CppDataFilter
        from_c(const DataFilter& df);
        [[nodiscard]] DataFilter
        to_c() const;

        static void
        cleanup_c(DataFilter& df);
    };

    struct CppDataRowUpdate {
        std::vector<std::pair<catalog::CppMetaColumn, unique_void_ptr>> assignments;

        // Constructor with table schema
        explicit CppDataRowUpdate(const CppMetaTable& table);
        
        static CppDataRowUpdate
        from_c(const DataRowUpdate& update, const CppMetaTable& table);
        [[nodiscard]] DataRowUpdate
        to_c() const;

        static void
        cleanup_c(DataRowUpdate& update);
        
    private:
        CppMetaTable table_schema_;
    };
}