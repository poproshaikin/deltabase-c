//
// Created by poproshaikin on 09.11.25.
//

#include "include/data_row.hpp"

namespace types
{
    DataRow::DataRow(const std::vector<SqlToken>& sql_tokens) : row_id(0), flags()
    {
        for (const auto& sql_token : sql_tokens)
        {
            tokens.push_back(DataToken(sql_token));
        }
    }
}