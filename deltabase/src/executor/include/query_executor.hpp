#ifndef QUERY_EXECUTOR_HPP
#define QUERY_EXECUTOR_HPP

#include "../../sql/include/parser.hpp"
#include "../../meta/include/meta_registry.hpp"
#include <memory>
#include <optional>
#include <string>
#include <variant>

extern "C" {
#include "../../core/include/data.h"
}

namespace exe {
    using IntOrDataTable = std::variant<int, std::unique_ptr<DataTable>>;

    enum class IsSupportedType { SUPPORTS = 0, DB_NAME_REQUIRED, UNSUPPORTED_STATEMENT };

    class IQueryExecutor {
      protected:
        meta::MetaRegistry& registry;
        IQueryExecutor(meta::MetaRegistry& registry) : registry(registry) {
        }

      public:
        virtual ~IQueryExecutor() = 0;

        virtual IsSupportedType
        supports(const sql::AstNodeType& type) const = 0;

        virtual IntOrDataTable
        execute(const sql::AstNode& node) = 0;

        virtual void
        set_db_name(std::string db_name) {};
    };

    class DatabaseExecutor : public IQueryExecutor {
        std::string db_name;

        std::unique_ptr<DataTable>
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
        DatabaseExecutor(meta::MetaRegistry& registry, std::string db_name)
            : db_name(db_name), IQueryExecutor(registry) {
        }

        IsSupportedType
        supports(const sql::AstNodeType& type) const override;

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
        AdminExecutor(meta::MetaRegistry& registry, std::optional<std::string> db_name)
            : db_name(db_name), IQueryExecutor(registry) {
        }

        IsSupportedType
        supports(const sql::AstNodeType& type) const override;

        IntOrDataTable
        execute(const sql::AstNode& node) override;

        void
        set_db_name(std::string db_name) override;
    };
} // namespace exe

#endif
