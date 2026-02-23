//
// Created by poproshaikin on 20.11.25.
//

#ifndef DELTABASE_DATA_TABLE_HPP
#define DELTABASE_DATA_TABLE_HPP

#include "data_row.hpp"

namespace types
{
    struct OutputColumn
    {
        std::string name;
        DataType type;
    };

    using OutputSchema = std::vector<OutputColumn>;

    struct DataTable
    {
        std::string table_name;

        OutputSchema output_schema;
        std::vector<DataRow> rows;
    };
}

#endif //DELTABASE_DATA_TABLE_HPP