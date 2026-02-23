//
// Created by poproshaikin on 02.01.26.
//

#ifndef DELTABASE_RESULTFORMATTER_HPP
#define DELTABASE_RESULTFORMATTER_HPP
#include "execution_result.hpp"
#include "data_token.hpp"

#include <string>
#include <vector>
#include <sstream>

namespace cli
{
    class ResultFormatter
    {
        std::string
        format_token(const types::DataToken& token) const;

        void
        draw_border(std::ostringstream& oss, const std::vector<size_t>& col_widths, bool is_top);

    public:
        std::string
        format(types::IExecutionResult& result);
    };
}

#endif //DELTABASE_RESULTFORMATTER_HPP