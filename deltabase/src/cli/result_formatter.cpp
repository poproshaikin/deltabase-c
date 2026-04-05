//
// Created by poproshaikin on 02.01.26.
//

#include "result_formatter.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace cli
{
    std::string
    ResultFormatter::format(types::IExecutionResult& result)
    {
        std::vector<types::DataRow> rows;
        types::DataRow row;
        
        while (result.next(row))
        {
            rows.push_back(row);
        }

        if (rows.empty())
        {
            return "No results.\n";
        }

        size_t num_columns = result.output_schema().size();
        std::vector<size_t> col_widths(num_columns, 0);

        for (const auto& r : rows)
        {
            for (size_t i = 0; i < r.tokens.size() && i < num_columns; ++i)
            {
                size_t len = format_token(r.tokens[i]).length();
                col_widths[i] = std::max(col_widths[i], len);
            }
        }

        std::ostringstream oss;

        draw_border(oss, col_widths, true);

        for (const auto& r : rows)
        {
            oss << "│";
            for (size_t i = 0; i < num_columns; ++i)
            {
                std::string value = i < r.tokens.size() ? format_token(r.tokens[i]) : "";
                oss << " " << std::left << std::setw(col_widths[i]) << value << " │";
            }
            oss << "\n";
        }

        draw_border(oss, col_widths, false);

        oss << rows.size() << (rows.size() == 1 ? " row" : " rows") << "\n";

        return oss.str();
    }

    std::string
    ResultFormatter::format_token(const types::DataToken& token) const
    {
        switch (token.type)
        {
        case types::DataType::INTEGER:
            return std::to_string(token.as<int>());
        case types::DataType::REAL:
            {
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(2) << token.as<double>();
                return oss.str();
            }
        case types::DataType::CHAR:
            return std::string(1, token.as<char>());
        case types::DataType::BOOL:
            return token.as<bool>() ? "true" : "false";
        case types::DataType::STRING:
            return token.as<std::string>();
        case types::DataType::_NULL:
        case types::DataType::UNDEFINED:
        default:
            return "NULL";
        }
    }

    void
    ResultFormatter::draw_border(std::ostringstream& oss, const std::vector<size_t>& col_widths, bool is_top)
    {
        oss << (is_top ? "┌" : "└");
        for (size_t i = 0; i < col_widths.size(); ++i)
        {
            if (i > 0)
            {
                oss << (is_top ? "┬" : "┴");
            }
            oss << std::string(col_widths[i] + 2, '-');
        }
        oss << (is_top ? "┐" : "┘") << "\n";
    }
}
