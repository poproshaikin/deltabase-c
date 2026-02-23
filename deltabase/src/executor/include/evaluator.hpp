//
// Created by poproshaikin on 20.11.25.
//

#ifndef DELTABASE_EVALUATOR_HPP
#define DELTABASE_EVALUATOR_HPP
#include "../../types/include/data_row.hpp"

namespace exq
{
    class Evaluator
    {
        const types::MetaTable table_;

        bool
        evaluate(const types::DataToken& left,
                 const types::DataToken& right,
                 types::AstOperator op) const;

        bool
        eq(const types::DataToken& left, const types::DataToken& right) const;
        bool
        lt(const types::DataToken& left, const types::DataToken& right) const;
        bool
        lte(const types::DataToken& left, const types::DataToken& right) const;
        bool
        gr(const types::DataToken& left, const types::DataToken& right) const;
        bool
        gre(const types::DataToken& left, const types::DataToken& right) const;

        bool
        eq(int left, int right) const;
        bool
        eq(double left, double right) const;
        bool
        eq(const std::string& left, const std::string& right) const;
        bool
        eq(char left, char right) const;
        bool
        eq(bool left, bool right) const;

        bool
        lt(int left, int right) const;
        bool
        lt(double left, double right) const;
        bool
        lt(const std::string& left, const std::string& right) const;
        bool
        lt(char left, char right) const;

        bool
        lte(int left, int right) const;
        bool
        lte(double left, double right) const;
        bool
        lte(const std::string& left, const std::string& right) const;
        bool
        lte(char left, char right) const;

        bool
        gr(int left, int right) const;
        bool
        gr(double left, double right) const;
        bool
        gr(const std::string& left, const std::string& right) const;
        bool
        gr(char left, char right) const;

        bool
        gre(int left, int right) const;
        bool
        gre(double left, double right) const;
        bool
        gre(const std::string& left, const std::string& right) const;
        bool
        gre(char left, char right) const;

    public:
        explicit
        Evaluator(const types::MetaTable& table);

        bool
        evaluate(const types::DataRow& row,
                 const types::BinaryExpr& expr) const;
    };
}

#endif //DELTABASE_EVALUATOR_HPP