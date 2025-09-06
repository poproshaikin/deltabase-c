#include "include/executor.hpp"

extern "C" {
#include "../../core/include/core.h"
#include "../../core/include/data.h"
}

namespace exe {

    ActionExecutionResult::ActionExecutionResult(IntOrDataTable&& result) 
        : success(true), result(std::forward<IntOrDataTable>(result)) {
    }

    ActionExecutionResult::ActionExecutionResult(std::pair<std::string, ActionError> error)
        : success(false), error(error) {
    }

    QueryPlanExecutionResult::QueryPlanExecutionResult(ActionExecutionResult&& result)
        : success(true), final_result(std::move(result.result)) {
    }

    QueryPlanExecutionResult::QueryPlanExecutionResult(IntOrDataTable&& result)
        : success(true), final_result(std::move(result)) {
    }

    QueryPlanExecutionResult::QueryPlanExecutionResult(std::pair<std::string, ActionError> error) 
        : success(false), error(error) {
    }

    ActionExecutor::ActionExecutor(engine::EngineConfig cfg, catalog::MetaRegistry& registry)
        : cfg_(cfg), registry_(registry) {
    }

    // Plans execution

    QueryPlanExecutionResult
    ActionExecutor::execute_plan(QueryPlan&& plan) {
        return std::visit([this](auto&& a) {
            return this->execute_plan(std::forward<decltype(a)>(a));
        }, std::move(plan));
    }

    QueryPlanExecutionResult
    ActionExecutor::execute_plan(SingleActionPlan&& plan) noexcept{
        return QueryPlanExecutionResult(execute_action(plan.action));
    }

    QueryPlanExecutionResult
    ActionExecutor::execute_plan(TransactionPlan&& plan) {
        throw std::runtime_error("Transactions arent supported yet");
    }

    ActionExecutionResult
    ActionExecutor::execute_action(const Action& action) noexcept {
        return std::visit([this](const auto& a) {
            return this->execute_action(a);
        }, action);
    }

    ActionExecutionResult
    ActionExecutor::execute_action(const SeqScanAction& action) noexcept {
        const auto& schema = registry_.get_schema_by_id(action.table.schema_id);
        MetaTable c_meta_table = action.table.to_c();

        size_t columns_count = action.table.columns.size();

        char** column_names = (char**)std::malloc(columns_count * sizeof(char*));
        if (!column_names) {
            return ActionExecutionResult(
                std::make_pair(
                    "Seq scan execution error: failed to allocate memory", ActionError::SYSTEM_ERROR
                )
            );
        }
        for (size_t i = 0; i < columns_count; i++) {
            column_names[i] = const_cast<char*>(action.columns[i].c_str());
        }

        std::unique_ptr<DataFilter> filter;
        if (action.filter) {
            filter = std::make_unique<DataFilter>(action.filter->to_c());
        }


        DataTable result;
        if (seq_scan(
                cfg_.db_name.value().c_str(),
                schema.name.c_str(),
                &c_meta_table,
                (const char**)column_names,
                columns_count,
                filter.get(),
                &result
            ) != 0) {
            return ActionExecutionResult(
                std::make_pair(
                    "Seq scan execution error: core function returned error", ActionError::SYSTEM_ERROR
                )
            );
        }

        result.schema = c_meta_table;
        return ActionExecutionResult(std::make_unique<catalog::CppDataTable>(std::move(result)));
    }

    ActionExecutionResult
    ActionExecutor::execute_action(const InsertAction& action) noexcept {
        auto c_table = action.table.to_c();
        auto c_schema = action.schema.to_c();
        auto c_row = action.row.to_c();

        if (insert_row(cfg_.db_name.value().c_str(), &c_schema, &c_table, &c_row) != 0) {
            return ActionExecutionResult(
                std::make_pair(
                    "Insert action execution error: core function returned error", ActionError::CORE_ERROR
                )
            );
        }

        catalog::CppMetaTable::cleanup_c(c_table);
        catalog::CppMetaSchema::cleanup_c(c_schema);
        catalog::CppDataRow::cleanup_c(c_row);

        return ActionExecutionResult(1lu);
    }

    ActionExecutionResult
    ActionExecutor::execute_action(const UpdateByFilterAction& action) noexcept {
        auto c_table = action.table.to_c();
        auto c_update = action.row_update.to_c();
        DataFilter* filter = nullptr;

        if (action.filter.has_value()) {
            *filter = action.filter->to_c();
        }

        size_t rows_affected = 0;
        if (update_rows_by_filter(
            cfg_.db_name->c_str(),
            action.schema.name.c_str(),
            &c_table,
            filter,
            &c_update,
            &rows_affected
        ) != 0) {
            return ActionExecutionResult(
                std::make_pair(
                    "Update by filter action execution error: core function returned error", ActionError::CORE_ERROR
                )
            );
        }

        catalog::CppMetaTable::cleanup_c(c_table);
        catalog::CppDataRowUpdate::cleanup_c(c_update);

        if (filter) {
            catalog::CppDataFilter::cleanup_c(*filter);
            std::free(filter);
        }

        return ActionExecutionResult(rows_affected);
    }

    ActionExecutionResult
    ActionExecutor::execute_action(const DeleteByFilterAction& action) noexcept {
        auto c_table = action.table.to_c();

        DataFilter* filter = nullptr;
        if (action.filter.has_value()) {
            *filter = action.filter->to_c();
        }

        size_t rows_affected = 0;
        if (::delete_rows_by_filter(
            cfg_.db_name->c_str(),
            action.schema.name.c_str(),
            &c_table,
            filter,
            &rows_affected
        ) != 0) {
            return ActionExecutionResult(
                std::make_pair(
                    "Delete by filter action execution error: core function returned error", ActionError::CORE_ERROR
                )
            );
        }

        if (filter) {
            catalog::CppDataFilter::cleanup_c(*filter);
            std::free(filter);
        }

        return ActionExecutionResult(rows_affected);
    }

    ActionExecutionResult
    ActionExecutor::execute_action(const CreateTableAction& action) noexcept {
        auto c_schema = action.schema.to_c();
        auto c_table = action.table.to_c();

        if (create_table(
            cfg_.db_name->c_str(),
            &c_schema,
            &c_table
        ) != 0) {
            return ActionExecutionResult(
                std::make_pair(
                    "Create table action execution error: core function returned error", ActionError::CORE_ERROR
                )
            );
        }

        catalog::CppMetaSchema::cleanup_c(c_schema);
        catalog::CppMetaTable::cleanup_c(c_table);

        return ActionExecutionResult(0lu);
    }

    ActionExecutionResult
    ActionExecutor::execute_action(const CreateSchemaAction& action) noexcept {
        auto c_schema = action.schema.to_c();

        if (create_schema(cfg_.db_name->c_str(), &c_schema) != 0) {
            return ActionExecutionResult(
                std::make_pair(
                    "Create schema action execution error: core function returned error", ActionError::CORE_ERROR
                )
            );
        }

        catalog::CppMetaSchema::cleanup_c(c_schema);
        return ActionExecutionResult(0lu);
    }

    ActionExecutionResult
    ActionExecutor::execute_action(const CreateDatabaseAction& action) noexcept {
        if (create_database(action.name.c_str()) != 0) {
            return ActionExecutionResult(
                std::make_pair(
                    "Create database action execution error: core function returned error", ActionError::CORE_ERROR
                )
            );
        }

        return ActionExecutionResult(0lu);
    }

    ActionExecutionResult
    ActionExecutor::execute_action(const WriteMetaTableAction& action) noexcept {
        try {
            registry_.add_table(action.table);
            return ActionExecutionResult(1lu);
        } catch (const std::exception& e) {
            return ActionExecutionResult(std::make_pair(
                std::string("Failed to write meta table: ") + e.what(), 
                ActionError::UNDEFINED
            ));
        }
    }

    ActionExecutionResult
    ActionExecutor::execute_action(const WriteMetaSchemaAction& action) noexcept {
        try {
            registry_.add_schema(action.schema);
            return ActionExecutionResult(1lu);
        } catch (const std::exception& e) {
            return ActionExecutionResult(std::make_pair(
                std::string("Failed to write meta schema: ") + e.what(), 
                ActionError::UNDEFINED
            ));
        }
    }
}