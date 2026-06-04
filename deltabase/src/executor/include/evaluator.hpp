//
// Created by poproshaikin on 20.11.25.
//

#ifndef DELTABASE_EVALUATOR_HPP
#define DELTABASE_EVALUATOR_HPP
#include "../../types/include/data_row.hpp"
#include "meta_table.hpp"

namespace exq
{
    class Evaluator
    {
        const types::MetaTable table_;

        bool
        evaluate(const types::DataToken& left,
                 const types::DataToken& right,
                 types::AstOperator op) const;

    public:
        explicit
        Evaluator(const types::MetaTable& table);

        bool
        evaluate(
            const types::MetaTable& table, const types::DataRow& row, const types::BinaryExpr& expr
        ) const;
    };
}

#endif //DELTABASE_EVALUATOR_HPP
