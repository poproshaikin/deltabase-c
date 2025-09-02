#ifndef QUERY_EXECUTOR_HPP
#define QUERY_EXECUTOR_HPP

#include "../../sql/include/parser.hpp"
#include "../../engine/include/config.hpp"
#include "../../catalog/include/meta_registry.hpp"
#include "../../catalog/include/data_object.hpp"
#include <memory>
#include <variant>

namespace exe {
    using IntOrDataTable = std::variant<int, std::unique_ptr<catalog::CppDataTable>>;

    class IQueryExecutor {
    protected:
        catalog::MetaRegistry& registry;
        const engine::EngineConfig cfg;  // Changed from reference to copy

        IQueryExecutor(catalog::MetaRegistry& registry, const engine::EngineConfig& cfg);

    public:
        virtual ~IQueryExecutor() = 0;

        virtual auto
        execute(const sql::AstNode& node) -> IntOrDataTable = 0;
    };

    class DatabaseExecutor : public IQueryExecutor {
        [[nodiscard]] auto
        execute_select(const sql::SelectStatement& stmt) -> std::unique_ptr<catalog::CppDataTable>;
        auto
        execute_insert(const sql::InsertStatement& stmt) -> int;
        auto
        execute_update(const sql::UpdateStatement& stmt) -> int;
        auto
        execute_delete(const sql::DeleteStatement& stmt) -> int;
        auto
        execute_create_table(const sql::CreateTableStatement& stmt) -> int;
        auto 
        execute_create_schema(const sql::CreateSchemaStatement& stmt) -> int;

    public:
        DatabaseExecutor() = delete;
        DatabaseExecutor(catalog::MetaRegistry& registry, const engine::EngineConfig& cfg);
        DatabaseExecutor(DatabaseExecutor&& other);

        auto
        execute(const sql::AstNode& node) -> IntOrDataTable override;
    };

    class AdminExecutor : public IQueryExecutor {
        auto
        execute_create_database(const sql::CreateDbStatement& stmt) -> int;

    public:
        AdminExecutor() = delete;
        AdminExecutor(catalog::MetaRegistry& registry, const engine::EngineConfig& cfg);
        AdminExecutor(AdminExecutor&& other);

        auto
        execute(const sql::AstNode& node) -> IntOrDataTable override;
    };

    class VirtualExecutor : public IQueryExecutor {
        auto
        execute_information_schema_tables() -> std::unique_ptr<catalog::CppDataTable>;
        auto
        execute_information_schema_columns() -> std::unique_ptr<catalog::CppDataTable>;

    public:
        VirtualExecutor() = delete;
        VirtualExecutor(catalog::MetaRegistry& registry, const engine::EngineConfig& cfg);
        VirtualExecutor(VirtualExecutor&& other);

        auto
        execute(const sql::AstNode& node) -> IntOrDataTable override;
    };
} // namespace exe

#endif
