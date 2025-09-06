#pragma once

#include "action.hpp"
#include "plan.hpp"

#include "../../catalog/include/registry.hpp"
#include "../../engine/include/config.hpp"

namespace exe {

    enum class ActionType {
        UNDEFINED = 0,
        SEQ_SCAN,
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
        catalog::MetaRegistry& registry_;

        // --- Actions ---
        
        ActionExecutionResult
        execute_action(const SeqScanAction& action);

        ActionExecutionResult
        execute_action(const InsertAction& action);
        
        ActionExecutionResult
        execute_action(const UpdateByFilterAction& action);

        ActionExecutionResult
        execute_action(const DeleteByFilterAction& action);

        ActionExecutionResult
        execute_action(const WriteMetaTableAction& action);

        ActionExecutionResult
        execute_action(const WriteMetaSchemaAction& action);

        ActionExecutionResult
        execute_action(const CreateTableAction& action);

        ActionExecutionResult
        execute_action(const CreateSchemaAction& action);

        ActionExecutionResult
        execute_action(const CreateDatabaseAction& action);

        ActionExecutionResult
        execute_action(const Action& action);

        // --- Plans ---

        QueryPlanExecutionResult
        execute_plan(SingleActionPlan&& plan);

        QueryPlanExecutionResult
        execute_plan(TransactionPlan&& plan);

    public:
        ActionExecutor(engine::EngineConfig cfg_, catalog::MetaRegistry& registry);

        QueryPlanExecutionResult
        execute_plan(QueryPlan&& plan);
    };
}