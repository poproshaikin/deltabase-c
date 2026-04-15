//
// Created by poproshaikin on 4/13/26.
//

#ifndef DELTABASE_QUERY_RESULT_SERIALIZER_HPP
#define DELTABASE_QUERY_RESULT_SERIALIZER_HPP
#include "typedefs.hpp"
#include "execution_result.hpp"
#include <vector>

namespace net
{
    class QueryResultSerializer
    {
    public:
        types::Bytes
        serialize(types::IExecutionResult& result) const;

        bool
        deserialize(
            const types::Bytes& bytes,
            types::OutputSchema& out_schema,
            std::vector<types::DataRow>& out_rows
        ) const;
    };
} // namespace net

#endif // DELTABASE_QUERY_RESULT_SERIALIZER_HPP
