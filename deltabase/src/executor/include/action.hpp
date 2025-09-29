#pragma once

#include "../../storage/include/objects/meta_object.hpp"
#include "../../storage/include/objects/data_object.hpp"

#include <optional>

namespace exe {
    using IntOrDataTable = std::variant<size_t, std::unique_ptr<storage::data_table>>;

    struct SeqScanAction {
        storage::meta_table& table;
        std::vector<std::string> columns;
        std::optional<storage::data_filter> filter;

        SeqScanAction(const storage::meta_table& table);
    };

    struct InsertAction {
        storage::meta_table& table;
        storage::meta_schema& schema;
        storage::data_row row;
    };

    struct UpdateByFilterAction {
        storage::meta_table& table;
        storage::meta_schema& schema;
        storage::data_row_update row_update;
        std::optional<storage::data_filter> filter;
    };

    struct DeleteByFilterAction {
        storage::meta_schema& schema;
        storage::meta_table& table;
        std::optional<storage::data_filter> filter;
    };

    struct CreateTableAction {
        storage::meta_schema& schema;
        storage::meta_table& table;
    };

    struct CreateSchemaAction {
        storage::meta_schema& schema;
    };

    struct CreateDatabaseAction {
        std::string name;
    };

    struct WriteMetaTableAction {
        storage::meta_table& table;
    };

    struct WriteMetaSchemaAction {
        storage::meta_schema& schema;
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