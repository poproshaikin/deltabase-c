//
// Created by poproshaikin on 02.01.26.
//

#ifndef DELTABASE_CLI_META_COMMAND_HPP
#define DELTABASE_CLI_META_COMMAND_HPP
#include <string>
#include <concepts>
#include <variant>

namespace cli
{
    enum class CommandType
    {
        SQL = 1,
        EXIT,
        CONNECT
    };

    inline bool
    is_sql_command(CommandType type)
    {
        return type == CommandType::SQL;
    }

    inline bool
    is_meta_command(CommandType type)
    {
        return type != CommandType::SQL;
    }

    struct ExitCommand
    {
        static constexpr auto type = CommandType::EXIT;
    };

    struct ConnectCommand
    {
        static constexpr auto type = CommandType::CONNECT;
        std::string db_name;
    };

    struct SqlCommand
    {
        static constexpr auto type = CommandType::SQL;
        std::string query;
    };

    template <typename T>
    concept CliCommand_c = requires {
        { T::type } -> std::convertible_to<CommandType>;
    };

    template <CliCommand_c... Ts>
    using CliCommandVariant = std::variant<Ts...>;

    using CliCommand = CliCommandVariant<
        ExitCommand,
        ConnectCommand,
        SqlCommand
    >;
}

#endif //DELTABASE_CLI_META_COMMAND_HPP