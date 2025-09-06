#include "include/executor.hpp"

extern "C" {
#include "../../core/include/core.h"
#include "../../core/include/data.h"
}

#include <format>

namespace exe {

    ActionExecutionResult::ActionExecutionResult(IntOrDataTable&& result) 
        : success(true), result(std::forward<IntOrDataTable>(result)) {
    }

    ActionExecutionResult::ActionExecutionResult(std::pair<std::string, ActionError> error)
        : success(false), error(error) {
    }

    ActionExecutor::ActionExecutor(engine::EngineConfig cfg, catalog::MetaRegistry& registry)
        : cfg_(cfg), registry_(registry) {
    }

    QueryPlanExecutionResult
    ActionExecutor::execute_plan(QueryPlan&& plan) {
        return std::visit([this](auto&& a) {
            return this->execute_plan(std::forward<decltype(a)>(a));
        }, std::move(plan));
    }

    QueryPlanExecutionResult
    ActionExecutor::execute_plan(SingleActionPlan&& plan) {
        return QueryPlanExecutionResult(execute_action(plan.action));
    }

    QueryPlanExecutionResult
    ActionExecutor::execute_plan(TransactionPlan&& plan) {
        throw std::runtime_error("Transactions arent supported yet");
    }

    ActionExecutionResult
    ActionExecutor::execute_action(const Action& action) {
        return std::visit([this](const auto& a) {
            return this->execute_action(a);
        }, action);
    }

    ActionExecutionResult
    ActionExecutor::execute_action(const SeqScanAction& action) {
        const auto& schema = registry_.get_schema_by_id(action.table.schema_id);
        MetaTable c_meta_table = action.table.to_c();

        size_t columns_count = action.table.columns.size();

        char** column_names = (char**)std::malloc(columns_count * sizeof(char*));
        if (!column_names) {
            throw std::runtime_error("Failed to allocate memory in execute_query");
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
            throw std::runtime_error("Failed to scan a table");
        }

        result.schema = c_meta_table;
        return ActionExecutionResult(std::make_unique<catalog::CppDataTable>(std::move(result)));
    }

    ActionExecutionResult
    ActionExecutor::execute_action(const InsertAction& action) {
        auto c_table = action.table.to_c();
        auto c_schema = action.schema.to_c();
        auto c_row = action.row.to_c();

        if (insert_row(cfg_.db_name.value().c_str(), &c_schema, &c_table, &c_row) != 0) {
            throw std::runtime_error("In execute_action: failed to insert row");
        }

        catalog::CppMetaTable::cleanup_c(c_table);
        catalog::CppMetaSchema::cleanup_c(c_schema);
        catalog::CppDataRow::cleanup_c(c_row);

        return ActionExecutionResult(1lu);
    }

    ActionExecutionResult
    ActionExecutor::execute_action(const UpdateByFilterAction& action) {
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
            throw std::runtime_error("In execute_action: failed to update rows by filter");
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
    ActionExecutor::execute_action(const DeleteByFilterAction& action) {
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
            throw std::runtime_error("In execute_action: failed to delete rows by filter");
        }

        if (filter) {
            catalog::CppDataFilter::cleanup_c(*filter);
            std::free(filter);
        }

        return ActionExecutionResult(rows_affected);
    }

    ActionExecutionResult
    ActionExecutor::execute_action(const CreateTableAction& action) {
        auto c_schema = action.schema.to_c();
        auto c_table = action.table.to_c();

        if (create_table(
            cfg_.db_name->c_str(),
            &c_schema,
            &c_table
        ) != 0) {
            throw std::runtime_error(std::format("In execute_action: failed to create table '{}'", action.table.name));
        }

        catalog::CppMetaSchema::cleanup_c(c_schema);
        catalog::CppMetaTable::cleanup_c(c_table);

        return ActionExecutionResult(0lu);
    }

    ActionExecutionResult
    ActionExecutor::execute_action(const CreateSchemaAction& action) {
        auto c_schema = action.schema.to_c();

        if (create_schema(cfg_.db_name->c_str(), &c_schema) != 0) {
            throw std::runtime_error("In execute_action: failed to create schema");
        }

        catalog::CppMetaSchema::cleanup_c(c_schema);
        return ActionExecutionResult(0lu);
    }

    ActionExecutionResult
    ActionExecutor::execute_action(const CreateDatabaseAction& action) {
        if (create_database(action.name.c_str()) != 0) {
            throw std::runtime_error("In execute_action: failed to create database " + action.name);
        }

        return ActionExecutionResult(0lu);
    }

}