//
// Created by poproshaikin on 10.11.25.
//

#include "parser.hpp"
#include "include/std_database.hpp"
#include "../sql/include/lexer.hpp"

namespace engine
{
    StdDatabase::StdDatabase(const std::string& db_name, const types::DbConfig& db_config)
        : db_name_(db_name), db_config_(db_config)
    {
    }

    std::unique_ptr<types::IExecutionResult>
    StdDatabase::execute(const std::string& query)
    {
        auto tokens = sql::lex(query);
        if (tokens.empty())
            return nullptr;

        sql::SqlParser parser(tokens);
        auto ast = parser.parse();

        auto plan = planner_->plan(std::move(ast));
        auto result = plan_executor_->execute(std::move(plan));

        return result;
    }
}