//
// Created by poproshaikin on 20.11.25.
//

#include "include/evaluator.hpp"

#include <iostream>

namespace exq
{
    using namespace types;

    Evaluator::Evaluator(const MetaTable& table) : table_(table)
    {
    }

    bool
    Evaluator::evaluate(const DataRow& row, const BinaryExpr& expr) const
    {
        auto left = std::get<SqlToken>(expr.left->value);
        auto right = std::get<SqlToken>(expr.right->value);

        if (right.is_literal())
        {
            DataToken left_literal(left);
            DataToken right_literal(right);

            return evaluate(left_literal, right_literal, expr.op);
        }
        if (right.is_identifier())
        {
            int64_t col_idx = table_.get_column_idx(right);

            DataToken left_literal(left);
            DataToken right_literal(row.tokens[col_idx]);

            return evaluate(left_literal, right_literal, expr.op);
        }

        return false;
    }

    bool
    Evaluator::evaluate(const DataToken& left,
                        const DataToken& right,
                        AstOperator op) const
    {
        switch (op)
        {
        case AstOperator::EQ:
            return eq(left, right);
        case AstOperator::NEQ:
            return !eq(left, right);
        case AstOperator::LT:
            return lt(left, right);
        case AstOperator::LTE:
            return lte(left, right);
        case AstOperator::GR:
            return gr(left, right);
        case AstOperator::GRE:
            return gre(left, right);
        default:
            return false;
        }
    }

    bool
    Evaluator::eq(const DataToken& left, const DataToken& right) const
    {
        if (left.type != right.type)
        {
            // TODO
            std::cerr << "Evaluator::eq: Type mismatch. Returning false";
            return false;
        }

        switch (left.type)
        {
        case DataType::INTEGER:
            return eq(left.as<int>(), right.as<int>());

        case DataType::REAL:
            return eq(left.as<double>(), right.as<double>());

        case DataType::STRING:
            return eq(left.as<std::string>(), right.as<std::string>());

        case DataType::BOOL:
            return eq(left.as<bool>(), right.as<bool>());

        case DataType::CHAR:
            return eq(left.as<char>(), right.as<char>());

        default:
            return false;
        }
    }

    bool
    Evaluator::eq(const int left, const int right) const
    {
        return left == right;
    }

    bool
    Evaluator::eq(const double left, const double right) const
    {
        return left == right;
    }

    bool
    Evaluator::eq(const std::string& left, const std::string& right) const
    {
        return left == right;
    }

    bool
    Evaluator::eq(const bool left, const bool right) const
    {
        return left == right;
    }

    bool
    Evaluator::eq(const char left, const char right) const
    {
        return left == right;
    }

    bool
    Evaluator::lt(const DataToken& left, const DataToken& right) const
    {
        if (left.type != right.type)
        {
            // TODO
            std::cerr << "Evaluator::lt: Type mismatch. Returning false";
            return false;
        }

        switch (left.type)
        {
        case DataType::INTEGER:
            return lt(left.as<int>(), right.as<int>());

        case DataType::REAL:
            return lt(left.as<double>(), right.as<double>());

        case DataType::STRING:
            return lt(left.as<std::string>(), right.as<std::string>());

        case DataType::CHAR:
            return lt(left.as<char>(), right.as<char>());

        default:
            return false;
        }
    }

    bool
    Evaluator::lt(const int left, const int right) const
    {
        return left > right;
    }

    bool
    Evaluator::lt(const double left, const double right) const
    {
        return left > right;
    }

    bool
    Evaluator::lt(const std::string& left, const std::string& right) const
    {
        return left > right;
    }

    bool
    Evaluator::lt(const char left, const char right) const
    {
        return left > right;
    }

    bool
    Evaluator::lte(const DataToken& left, const DataToken& right) const
    {
        if (left.type != right.type)
        {
            // TODO
            std::cerr << "Evaluator::lte: Type mismatch. Returning false";
            return false;
        }

        switch (left.type)
        {
        case DataType::INTEGER:
            return lte(left.as<int>(), right.as<int>());

        case DataType::REAL:
            return lte(left.as<double>(), right.as<double>());

        case DataType::STRING:
            return lte(left.as<std::string>(), right.as<std::string>());

        case DataType::CHAR:
            return lte(left.as<char>(), right.as<char>());

        default:
            return false;
        }
    }

    bool
    Evaluator::lte(const int left, const int right) const
    {
        return left >= right;
    }

    bool
    Evaluator::lte(const double left, const double right) const
    {
        return left >= right;
    }

    bool
    Evaluator::lte(const std::string& left, const std::string& right) const
    {
        return left >= right;
    }

    bool
    Evaluator::lte(const char left, const char right) const
    {
        return left >= right;
    }

    bool
    Evaluator::gr(const DataToken& left, const DataToken& right) const
    {
        if (left.type != right.type)
        {
            // TODO
            std::cerr << "Evaluator::gr: Type mismatch. Returning false";
            return false;
        }

        switch (left.type)
        {
        case DataType::INTEGER:
            return gr(left.as<int>(), right.as<int>());

        case DataType::REAL:
            return gr(left.as<double>(), right.as<double>());

        case DataType::STRING:
            return gr(left.as<std::string>(), right.as<std::string>());

        case DataType::CHAR:
            return gr(left.as<char>(), right.as<char>());

        default:
            return false;
        }
    }

    bool
    Evaluator::gr(const int left, const int right) const
    {
        return left > right;
    }

    bool
    Evaluator::gr(const double left, const double right) const
    {
        return left > right;
    }

    bool
    Evaluator::gr(const std::string& left, const std::string& right) const
    {
        return left > right;
    }

    bool
    Evaluator::gr(const char left, const char right) const
    {
        return left > right;
    }

    bool
    Evaluator::gre(const DataToken& left, const DataToken& right) const
    {
        if (left.type != right.type)
        {
            // TODO
            std::cerr << "Evaluator::gre: Type mismatch. Returning false";
            return false;
        }

        switch (left.type)
        {
        case DataType::INTEGER:
            return gre(left.as<int>(), right.as<int>());

        case DataType::REAL:
            return gre(left.as<double>(), right.as<double>());

        case DataType::STRING:
            return gre(left.as<std::string>(), right.as<std::string>());

        case DataType::CHAR:
            return gre(left.as<char>(), right.as<char>());

        default:
            return false;
        }
    }

    bool
    Evaluator::gre(const int left, const int right) const
    {
        return left >= right;
    }

    bool
    Evaluator::gre(const double left, const double right) const
    {
        return left >= right;
    }

    bool
    Evaluator::gre(const std::string& left, const std::string& right) const
    {
        return left >= right;
    }

    bool
    Evaluator::gre(const char left, const char right) const
    {
        return left >= right;
    }
}