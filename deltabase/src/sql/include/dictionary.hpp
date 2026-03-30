//
// Created by poproshaikin on 02.12.25.
//

#ifndef DELTABASE_DICTIONARY_HPP
#define DELTABASE_DICTIONARY_HPP
#include <string>
#include <unordered_map>
#include "../../types/include/sql_token.hpp"
#include "../../types/include/data_type.hpp"

namespace sql
{
    using namespace types;

    inline const std::unordered_map<std::string, SqlKeyword>&
    keywords_map()
    {
        static const std::unordered_map<std::string, SqlKeyword> dictionary = {
            {"select", SqlKeyword::SELECT},
            {"from", SqlKeyword::FROM},
            {"insert", SqlKeyword::INSERT},
            {"into", SqlKeyword::INTO},
            {"values", SqlKeyword::VALUES},
            {"update", SqlKeyword::UPDATE},
            {"set", SqlKeyword::SET},
            {"delete", SqlKeyword::DELETE},
            {"where", SqlKeyword::WHERE},
            {"create", SqlKeyword::CREATE},
            {"drop", SqlKeyword::DROP},
            {"database", SqlKeyword::DATABASE},
            {"table", SqlKeyword::TABLE},
            {"string", SqlKeyword::STRING},
            {"integer", SqlKeyword::INTEGER},
            {"real", SqlKeyword::REAL},
            {"char", SqlKeyword::CHAR},
            {"bool", SqlKeyword::BOOL},
            {"null", SqlKeyword::_NULL},
            {"primary", SqlKeyword::PRIMARY},
            {"key", SqlKeyword::KEY},
            {"not", SqlKeyword::NOT},
            {"autoincrement", SqlKeyword::AUTOINCREMENT},
            {"unique", SqlKeyword::UNIQUE},
            {"schema", SqlKeyword::SCHEMA},
            {"index", SqlKeyword::INDEX},
            {"on", SqlKeyword::ON}
        };

        return dictionary;
    }

    inline const std::unordered_map<std::string, SqlKeyword>&
    data_types_map()
    {
        static const std::unordered_map<std::string, SqlKeyword> types_map = {
            {"string", SqlKeyword::STRING},
            {"integer", SqlKeyword::INTEGER},
            {"real", SqlKeyword::REAL},
            {"char", SqlKeyword::CHAR},
            {"bool", SqlKeyword::BOOL},
            {"null", SqlKeyword::_NULL}};

        return types_map;
    }

    inline const std::unordered_map<SqlKeyword, DataType>&
    kw_to_data_type_map()
    {
        static const std::unordered_map<SqlKeyword, DataType> types_map = {
            {SqlKeyword::INTEGER, DataType::INTEGER},
            {SqlKeyword::STRING, DataType::STRING},
            {SqlKeyword::REAL, DataType::REAL},
            {SqlKeyword::BOOL, DataType::BOOL},
            {SqlKeyword::CHAR, DataType::CHAR},
            {SqlKeyword::_NULL, DataType::_NULL},
        };

        return types_map;
    }

    inline bool
    is_data_type_kw(const SqlKeyword& kw)
    {
        const auto& types_map = data_types_map();
        for (const auto& [key, value] : types_map)
        {
            if (value == kw)
            {
                return true;
            }
        }
        return false;
    }

    inline DataType
    to_data_type(SqlKeyword kw)
    {
        const auto& types_map = kw_to_data_type_map();

        auto it = types_map.find(kw);
        if (it == types_map.end())
            return DataType::UNDEFINED;

        return it->second;
    }

    inline const std::unordered_map<std::string, SqlKeyword>&
    constraints_map()
    {
        static const std::unordered_map<std::string, SqlKeyword> constraints_map = {
            {"not", SqlKeyword::NOT},
            {"null", SqlKeyword::_NULL},
            {"primary", SqlKeyword::PRIMARY},
            {"key", SqlKeyword::KEY},
            {"autoincrement", SqlKeyword::AUTOINCREMENT},
            {"unique", SqlKeyword::UNIQUE},
        };

        return constraints_map;
    }

    inline bool
    is_constraint_kw(const SqlKeyword& kw)
    {
        const auto& constraints = constraints_map();
        for (const auto& [key, value] : constraints)
        {
            if (value == kw)
            {
                return true;
            }
        }
        return false;
    }

    inline const std::unordered_map<std::string, SqlSymbol>&
    symbols_map()
    {
        static const std::unordered_map<std::string, SqlSymbol> DICTIONARY = {
            {"(", SqlSymbol::LPAREN},
            {")", SqlSymbol::RPAREN},
            {",", SqlSymbol::COMMA},
            {".", SqlSymbol::PERIOD},
            {";", SqlSymbol::SEMICOLON}};
        return DICTIONARY;
    }

    inline const std::unordered_map<std::string, SqlOperator>&
    operators_map()
    {
        static const std::unordered_map<std::string, SqlOperator> DICTIONARY = {
            {"==", SqlOperator::EQ},
            {"!=", SqlOperator::NEQ},

            {">", SqlOperator::GR},
            {">=", SqlOperator::GRE},
            {"<", SqlOperator::LT},
            {"<=", SqlOperator::LTE},

            {"and", SqlOperator::AND},
            {"or", SqlOperator::OR},
            {"not", SqlOperator::NOT},

            {"+", SqlOperator::PLUS},
            {"-", SqlOperator::MINUS},
            {"*", SqlOperator::MUL},
            {"/", SqlOperator::DIV},

            {"=", SqlOperator::ASSIGN},
        };
        return DICTIONARY;
    }
}

#endif //DELTABASE_DICTIONARY_HPP