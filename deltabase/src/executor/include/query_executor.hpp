#ifndef QUERY_EXECUTOR_HPP
#define QUERY_EXECUTOR_HPP

#include "../../sql/include/parser.hpp"
#include "../../catalog/include/meta_registry.hpp"
#include "../../catalog/include/data_object.hpp"
#include <memory>
#include <optional>
#include <string>
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

        virtual IntOrDataTable
        execute(const sql::AstNode& node) = 0;

        virtual void
        set_db_name(std::string db_name) {};
    };

    class DatabaseExecutor : public IQueryExecutor {
        std::string db_name;

        std::unique_ptr<catalog::CppDataTable>
        execute_select(const sql::SelectStatement& stmt);

        int
        execute_insert(const sql::InsertStatement& stmt);

        int
        execute_update(const sql::UpdateStatement& stmt);

        int
        execute_delete(const sql::DeleteStatement& stmt);

        int
        execute_create_table(const sql::CreateTableStatement& stmt);

    public:
        DatabaseExecutor(catalog::MetaRegistry& registry, std::string db_name)
            : db_name(db_name), IQueryExecutor(registry) {
        }

        DatabaseExecutor(DatabaseExecutor&& other)
            : IQueryExecutor(other.registry), db_name(std::move(other.db_name)) {
        }

        IntOrDataTable
        execute(const sql::AstNode& node) override;
        
        void
        set_db_name(std::string db_name) override;
    };

    class AdminExecutor : public IQueryExecutor {
        std::optional<std::string> db_name;

        int
        execute_create_database(const sql::CreateDbStatement& stmt);

    public:
        AdminExecutor(catalog::MetaRegistry& registry, std::optional<std::string> db_name)
            : db_name(db_name), IQueryExecutor(registry) {
        }

        AdminExecutor(AdminExecutor&& other)
            : IQueryExecutor(other.registry), db_name(std::move(other.db_name)) {
        }

        IntOrDataTable
        execute(const sql::AstNode& node) override;

        void
        set_db_name(std::string db_name) override;
    };

    class VirtualExecutor : public IQueryExecutor {
        std::unique_ptr<catalog::CppDataTable>
        execute_information_schema_tables();

        std::unique_ptr<catalog::CppDataTable>
        execute_information_schema_columns();

    public:
        VirtualExecutor(catalog::MetaRegistry& registry) : IQueryExecutor(registry) {
        }

        VirtualExecutor(VirtualExecutor&& other) : IQueryExecutor(other.registry) {
        }

        IntOrDataTable
        execute(const sql::AstNode& node) override;
    };
} // namespace exe

#endif
