#include "include/executor.hpp"
#include "../storage/include/storage.hpp"
#include "include/plan.hpp"

namespace exe
{

    ActionExecutionResult::ActionExecutionResult(IntOrDataTable&& result)
        : success(true), result(std::forward<IntOrDataTable>(result))
    {
    }

    ActionExecutionResult::ActionExecutionResult(std::pair<std::string, ActionError> error)
        : success(false), error(error)
    {
    }

    QueryPlanExecutionResult::QueryPlanExecutionResult(ActionExecutionResult&& result)
        : success(true), final_result(std::move(result.result))
    {
    }

    QueryPlanExecutionResult::QueryPlanExecutionResult(IntOrDataTable&& result)
        : success(true), final_result(std::move(result))
    {
    }

    QueryPlanExecutionResult::QueryPlanExecutionResult(std::pair<std::string, ActionError> error)
        : success(false), error(error)
    {
    }

    ActionExecutor::ActionExecutor(const engine::EngineConfig& cfg, storage::Storage& storage)
        : cfg_(cfg), storage_(storage)
    {
    }

    // Plans execution

    QueryPlanExecutionResult
    ActionExecutor::execute_plan(QueryPlan&& plan)
    {
        return std::visit(
            [this](auto&& a) { return this->execute_plan(std::forward<decltype(a)>(a)); },
            std::move(plan)
        );
    }

    QueryPlanExecutionResult
    ActionExecutor::execute_plan(SingleActionPlan&& plan) noexcept
    {
        return QueryPlanExecutionResult(execute_action(plan.action));
    }

    QueryPlanExecutionResult
    ActionExecutor::execute_plan(TransactionPlan&& plan)
    {
        throw std::runtime_error("Transactions arent supported yet");
    }

    ActionExecutionResult
    ActionExecutor::execute_action(const Action& action) noexcept
    {
        return std::visit([this](const auto& a) { return this->execute_action(a); }, action);
    }

    ActionExecutionResult
    ActionExecutor::execute_action(const SeqScanAction& action) noexcept
    {
        try
        {
            auto result = storage_.seq_scan(action.table, action.columns, action.filter);
            return ActionExecutionResult(std::make_unique<storage::DataTable>(std::move(result)));
        }
        catch (const std::exception& e)
        {
            return {std::make_pair(
                std::string("Seq scan execution error: ") + e.what(), ActionError::SYSTEM_ERROR
            )};
        }
    }

    ActionExecutionResult
    ActionExecutor::execute_action(const InsertAction& action) noexcept
    {
        try
        {
            storage_.insert_row(action.table, action.row);
            return ActionExecutionResult(1lu);
        }
        catch (const std::exception& e)
        {
            return ActionExecutionResult(
                std::make_pair(
                    std::string("Insert execution error: ") + e.what(), ActionError::SYSTEM_ERROR
                )
            );
        }
    }

    ActionExecutionResult
    ActionExecutor::execute_action(const UpdateByFilterAction& action) noexcept
    {
        try
        {
            auto rows_affected = storage_.update_rows_by_filter(
                const_cast<storage::MetaTable&>(action.table),
                action.filter.value(), 
                action.row_update
            );
            return ActionExecutionResult(rows_affected);
        }
        catch (const std::exception& e)
        {
            return ActionExecutionResult(
                std::make_pair(
                    std::string("Update execution error: ") + e.what(), ActionError::SYSTEM_ERROR
                )
            );
        }
    }

    ActionExecutionResult
    ActionExecutor::execute_action(const DeleteByFilterAction& action) noexcept
    {
        try
        {
            auto rows_affected = storage_.delete_rows_by_filter(
                const_cast<storage::MetaTable&>(action.table), action.filter
            );
            return ActionExecutionResult(rows_affected);
        }
        catch (const std::exception& e)
        {
            return ActionExecutionResult(
                std::make_pair(
                    std::string("Delete execution error: ") + e.what(), ActionError::SYSTEM_ERROR
                )
            );
        }
    }

    ActionExecutionResult
    ActionExecutor::execute_action(const CreateTableAction& action) noexcept
    {
        try
        {
            storage_.create_table(std::move(action.table));
            return {0lu};
        }
        catch (const std::exception& e)
        {
            return {std::make_pair(
                std::string("Create table execution error: ") + e.what(), ActionError::SYSTEM_ERROR
            )};
        }
    }

    ActionExecutionResult
    ActionExecutor::execute_action(const CreateSchemaAction& action) noexcept
    {
        try
        {
            storage_.create_schema(std::move(action.schema));
            return {0lu};
        }
        catch (const std::exception& e)
        {
            return {std::make_pair(
                std::string("Create schema execution error: ") + e.what(), ActionError::SYSTEM_ERROR
            )};
        }
    }

    ActionExecutionResult
    ActionExecutor::execute_action(const CreateDatabaseAction& action) noexcept
    {
        try
        {
            storage_.create_database(action.name);
            return {0lu};
        }
        catch (const std::exception& e)
        {
            return {std::make_pair(
                std::string("Create database execution error: ") + e.what(),
                ActionError::SYSTEM_ERROR
            )};
        }
    }

    ActionExecutionResult
    ActionExecutor::execute_action(const WriteMetaTableAction& action) noexcept
    {
        try
        {
            // В новой архитектуре meta записи обрабатываются автоматически
            // через Storage при создании таблиц
            return ActionExecutionResult(1lu);
        }
        catch (const std::exception& e)
        {
            return ActionExecutionResult(
                std::make_pair(
                    std::string("Failed to write meta table: ") + e.what(), ActionError::UNDEFINED
                )
            );
        }
    }

    ActionExecutionResult
    ActionExecutor::execute_action(const WriteMetaSchemaAction& action) noexcept
    {
        try
        {
            // В новой архитектуре meta записи обрабатываются автоматически
            // через Storage при создании схем
            return ActionExecutionResult(1lu);
        }
        catch (const std::exception& e)
        {
            return ActionExecutionResult(
                std::make_pair(
                    std::string("Failed to write meta schema: ") + e.what(), ActionError::UNDEFINED
                )
            );
        }
    }
} // namespace exe