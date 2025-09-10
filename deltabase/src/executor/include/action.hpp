#pragma once

#include "../../storage/include/storage.hpp"

#include <optional>

namespace exe {
    using IntOrDataTable = std::variant<size_t, std::unique_ptr<storage::DataTable>>;

    struct SeqScanAction {
        storage::MetaTable table;
        std::vector<std::string> columns;
        std::optional<storage::DataFilter> filter;
    };

    struct InsertAction {
        storage::MetaTable table;
        storage::MetaSchema schema;
        storage::DataRow row;
    };

    struct UpdateByFilterAction {
        storage::MetaTable table;
        storage::MetaSchema schema;
        storage::DataRowUpdate row_update;
        std::optional<storage::DataFilter> filter;
    };

    struct DeleteByFilterAction {
        storage::MetaSchema schema;
        storage::MetaTable table;
        std::optional<storage::DataFilter> filter;
    };

    struct CreateTableAction {
        storage::MetaSchema schema;
        storage::MetaTable table;
    };

    struct CreateSchemaAction {
        storage::MetaSchema schema;
    };

    struct CreateDatabaseAction {
        std::string name;
    };

    struct WriteMetaTableAction {
        storage::MetaTable table;
    };

    struct WriteMetaSchemaAction {
        storage::MetaSchema schema;
    };

    using Action = std::variant<
        SeqScanAction,
        InsertAction,
        UpdateByFilterAction,
        DeleteByFilterAction,
        CreateTableAction,
        CreateSchemaAction,
        CreateDatabaseAction,
        WriteMetaTableAction,
        WriteMetaSchemaAction
    >;
}