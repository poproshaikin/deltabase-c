//
// Created by poproshaikin on 20.11.25.
//

#include "include/evaluator.hpp"

namespace exq
{
    using namespace types;

    Evaluator::Evaluator(const MetaTable& table) : table_(table)
    {
    }

    bool
    Evaluator::evaluate(const MetaTable& table, const DataRow& row, const BinaryExpr& expr) const
    {
        if (expr.op == AstOperator::AND)
        {
            return evaluate(table, row, std::get<BinaryExpr>(expr.left->value)) &&
                   evaluate(table, row, std::get<BinaryExpr>(expr.right->value));
        }
        if (expr.op == AstOperator::OR)
        {
            return evaluate(table, row, std::get<BinaryExpr>(expr.left->value)) ||
                   evaluate(table, row, std::get<BinaryExpr>(expr.right->value));
        }

        auto left  = std::get<SqlToken>(expr.left->value);
        auto right = std::get<SqlToken>(expr.right->value);

        if (left.is_identifier() && right.is_literal())
        {
            int64_t col_idx = table.get_column_idx(left);
            return evaluate(DataToken(row.tokens.at(col_idx)), DataToken(right), expr.op);
        }
        if (left.is_identifier() && right.is_identifier())
        {
            int64_t left_idx  = table_.get_column_idx(left);
            int64_t right_idx = table_.get_column_idx(right);
            return evaluate(DataToken(row.tokens[left_idx]), DataToken(row.tokens[right_idx]), expr.op);
        }
        if (left.is_literal() && right.is_literal())
        {
            return evaluate(DataToken(left), DataToken(right), expr.op);
        }

        throw std::runtime_error("Evaluator::evaluate: Invalid comparison");
    }

    bool
    Evaluator::evaluate(const DataToken& left, const DataToken& right, AstOperator op) const
    {
        // NULL semantics: EQ/IS treat NULL=NULL as true; ordered comparisons return false.
        if (left.type == DataType::_NULL || right.type == DataType::_NULL)
        {
            if (op == AstOperator::EQ || op == AstOperator::IS)
                return left.type == right.type;
            return false;
        }

        if (DataToken::common_type(left.type, right.type) == DataType::UNDEFINED)
            return false;

        const int c = DataToken::compare(left, right);

        switch (op)
        {
        case AstOperator::EQ:  return c == 0;
        case AstOperator::NEQ: return c != 0;
        case AstOperator::LT:  return c <  0;
        case AstOperator::LTE: return c <= 0;
        case AstOperator::GR:  return c >  0;
        case AstOperator::GRE: return c >= 0;
        case AstOperator::IS:  return c == 0;
        default:               return false;
        }
    }
}
