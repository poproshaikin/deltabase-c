#pragma once

#include "../../catalog/include/meta_object.hpp"
#include "../../catalog/include/data_object.hpp"

#include <optional>

namespace exe {
    using IntOrDataTable = std::variant<size_t, std::unique_ptr<catalog::CppDataTable>>;

    struct SeqScanAction {
        catalog::CppMetaTable table;
        std::vector<std::string> columns;
        std::unique_ptr<catalog::CppDataFilter> filter;
    };

    struct InsertAction {
        catalog::CppMetaTable table;
        catalog::CppMetaSchema schema;
        catalog::CppDataRow row;
    };

    struct UpdateByFilterAction {
        catalog::CppMetaTable table;
        catalog::CppMetaSchema schema;
        catalog::CppDataRowUpdate row_update;
        std::optional<catalog::CppDataFilter> filter;
    };

    struct DeleteByFilterAction {
        catalog::CppMetaSchema schema;
        catalog::CppMetaTable table;
        std::optional<catalog::CppDataFilter> filter;
    };

    struct CreateTableAction {
        catalog::CppMetaSchema schema;
        catalog::CppMetaTable table;
    };

    struct CreateSchemaAction {
        catalog::CppMetaSchema schema;
    };

    struct CreateDatabaseAction {
        std::string name;
    };

    struct WriteMetaTableAction {
        catalog::CppMetaTable table;
    };

    struct WriteMetaSchemaAction {
        catalog::CppMetaSchema schema;
    };

    using Action = std::variant<
        SeqScanAction,
        InsertAction,
        UpdateByFilterAction,
        DeleteByFilterAction,
        CreateTableAction,
        CreateSchemaAction,
        WriteMetaTableAction,
        WriteMetaSchemaAction
    >;
}