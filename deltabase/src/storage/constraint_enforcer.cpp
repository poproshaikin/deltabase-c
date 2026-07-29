//
// Created by poproshaikin on 6/19/26.
//

#include "constraint_enforcer.hpp"

#include "exceptions.hpp"

namespace storage
{
    using namespace types;

    ConstraintEnforcer::ConstraintEnforcer(DqlService& dql, const CatalogCache& catalog)
        : dql_(dql), catalog_(catalog)
    {
    }

    void
    ConstraintEnforcer::validate_or_throw(
        const MetaTable& mt,
        const std::vector<DataToken>& row)
    {
        for (size_t i = 0; i < mt.columns.size(); ++i)
        {
            const auto& col = mt.columns[i];
            const bool is_null = i >= row.size() || row[i].type == DataType::_NULL;

            if (col.has_constraint<MetaNotNullConstraint>() && is_null)
                throw EngineException(
                    "NOT NULL constraint violated for column '" + col.name + "'",
                    EngineException::Code::NOT_NULL_VIOLATION);

            if (mt.is_unique(col.name) && !is_null && dql_.value_exists(mt, col.name, row[i]))
            {
                throw EngineException(
                    "UNIQUE constraint violated for column '" + col.name + "'",
                    EngineException::Code::UNIQUE_VIOLATION);

            }

            if (col.has_constraint<MetaForeignKeyConstraint>())
            {
                auto fk = col.get_constraint<MetaForeignKeyConstraint>();
                const auto* referenced_mt = catalog_.get_table(fk->referenced_table_id);
                const auto& referenced_col_name = referenced_mt->get_column(
                    fk->referenced_column_id).name;

                if (!is_null && !dql_.value_exists(*referenced_mt, referenced_col_name, row[i]))
                    throw EngineException(
                        "FOREIGN KEY constraint violated for column '" + col.name +
                        "': this value doesn't exist in table " + referenced_mt->name,
                        EngineException::Code::FOREIGN_KEY_VIOLATION);
            }
        }
    }
}