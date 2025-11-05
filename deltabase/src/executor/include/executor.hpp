#pragma once

#include "action.hpp"
#include "plan.hpp"
#include "../../storage/include/storage.hpp"
#include "../../engine/include/config.hpp"

namespace exe {

    enum class ActionType {
        UNDEFINED = 0,
        SEQ_SCAN = 1,
        INSERT,
        UPDATE_BY_FILTER,
        DELETE_BY_FILTER,
        WRITE_META_TABLE,
        WRITE_META_SCHEMA,
        CREATE_TABLE,
        CREATE_SCHEMA,
        CREATE_DB
    };

    enum class ActionError {
        UNDEFINED = 0,
        SYSTEM_ERROR,
        CORE_ERROR
    };

    struct ActionExecutionResult {
        bool success;
        IntOrDataTable result;
        std::optional<std::pair<std::string, ActionError>> error;

        ActionExecutionResult(IntOrDataTable&& result);
        ActionExecutionResult(std::pair<std::string, ActionError> error);
    };

    struct QueryPlanExecutionResult {
        bool success;
        IntOrDataTable final_result;
        std::optional<std::pair<std::string, ActionError>> error;

        QueryPlanExecutionResult(ActionExecutionResult&& result);
        QueryPlanExecutionResult(IntOrDataTable&& result);
        QueryPlanExecutionResult(std::pair<std::string, ActionError> error);
    };

    class ActionExecutor {
        engine::EngineConfig cfg_;
        storage::Storage& storage_;

        // --- Actions ---
        
        ActionExecutionResult
        execute_action(const SeqScanAction& action) const noexcept;

        ActionExecutionResult
        execute_action(const InsertAction& action) const noexcept;
        
        ActionExecutionResult
        execute_action(const UpdateByFilterAction& action) const noexcept;

        ActionExecutionResult
        execute_action(const DeleteByFilterAction& action) const noexcept;

        ActionExecutionResult
        execute_action(const WriteMetaTableAction& action) noexcept;

        ActionExecutionResult
        execute_action(const WriteMetaSchemaAction& action) noexcept;

        ActionExecutionResult
        execute_action(const CreateTableAction& action) const noexcept;

        ActionExecutionResult
        execute_action(const CreateSchemaAction& action) const noexcept;

        ActionExecutionResult
        execute_action(const CreateDatabaseAction& action) const noexcept;

        ActionExecutionResult
        execute_action(const Action& action) const noexcept;

        // --- Plans ---

        QueryPlanExecutionResult
        execute_plan(SingleActionPlan&& plan) noexcept;

        QueryPlanExecutionResult
        execute_plan(TransactionPlan&& plan);

    public:
        ActionExecutor(const engine::EngineConfig& cfg, storage::Storage& storage);

        QueryPlanExecutionResult
        execute_plan(QueryPlan&& plan);
    };
}