#ifndef QUERY_EXECUTOR_HPP
#define QUERY_EXECUTOR_HPP

#include "../../sql/include/parser.hpp"
#include "../../catalog/include/meta_registry.hpp"
#include "../../catalog/include/data_object.hpp"
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace exe {
    using IntOrDataTable = std::variant<int, std::unique_ptr<catalog::CppDataTable>>;

    class IQueryExecutor {
    protected:
        catalog::MetaRegistry& registry;
        IQueryExecutor(catalog::MetaRegistry& registry) : registry(registry) {
        }

    public:
        virtual ~IQueryExecutor() = 0;

        virtual auto
        execute(const sql::AstNode& node) -> IntOrDataTable = 0;

        virtual void
        set_db_name(std::string db_name) {};
    };

    class DatabaseExecutor : public IQueryExecutor {
        std::string db_name_;

        auto
        execute_select(const sql::SelectStatement& stmt) -> std::unique_ptr<catalog::CppDataTable>;

        auto
        execute_insert(const sql::InsertStatement& stmt) -> int;

        auto
        execute_update(const sql::UpdateStatement& stmt) -> int;

        auto
        execute_delete(const sql::DeleteStatement& stmt) -> int;

        auto
        execute_create_table(const sql::CreateTableStatement& stmt) -> int;

    public:
        DatabaseExecutor(catalog::MetaRegistry& registry, std::string db_name)
            : db_name_(std::move(db_name)), IQueryExecutor(registry) {
        }

        DatabaseExecutor(DatabaseExecutor&& other)
            : IQueryExecutor(other.registry), db_name_(std::move(other.db_name_)) {
        }

        auto
        execute(const sql::AstNode& node) -> IntOrDataTable override;
        
        void
        set_db_name(std::string db_name) override;
    };

    class AdminExecutor : public IQueryExecutor {
        std::optional<std::string> db_name_;

        auto
        execute_create_database(const sql::CreateDbStatement& stmt) -> int;

    public:
        AdminExecutor(catalog::MetaRegistry& registry, std::optional<std::string> db_name)
            : db_name_(std::move(db_name)), IQueryExecutor(registry) {
        }

        AdminExecutor(AdminExecutor&& other)
            : IQueryExecutor(other.registry), db_name_(std::move(other.db_name_)) {
        }

        auto
        execute(const sql::AstNode& node) -> IntOrDataTable override;

        void
        set_db_name(std::string db_name) override;
    };

    class VirtualExecutor : public IQueryExecutor {
        auto
        execute_information_schema_tables() -> std::unique_ptr<catalog::CppDataTable>;

        auto
        execute_information_schema_columns() -> std::unique_ptr<catalog::CppDataTable>;

    public:
        VirtualExecutor(catalog::MetaRegistry& registry) : IQueryExecutor(registry) {
        }

        VirtualExecutor(VirtualExecutor&& other) : IQueryExecutor(other.registry) {
        }

        auto
        execute(const sql::AstNode& node) -> IntOrDataTable override;
    };
} // namespace exe

#endif
