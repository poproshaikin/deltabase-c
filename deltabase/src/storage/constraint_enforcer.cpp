//
// Created by poproshaikin on 6/19/26.
//

#include "constraint_enforcer.hpp"

#include "exceptions.hpp"

namespace storage
{
    using namespace types;

    ConstraintEnforcer::ConstraintEnforcer(DQLService& dql, DMLService& dml, CatalogCache& catalog)
        : dql_(dql), dml_(dml), catalog_(catalog)
    {
    }

    void
    ConstraintEnforcer::validate_or_throw(
        const MetaTable& mt,
        const std::vector<DataToken>& row) const
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

    void
    ConstraintEnforcer::handle_fk_on_delete(
        const MetaTable& deleted_from_table,
        MetaTable& current_table,
        const MetaForeignKeyConstraint& fk,
        const DataRow& deleting_row,
        const MetaColumn& col,
        txn::Transaction* txn)
    {
        int idx = deleted_from_table.get_column_idx(fk.referenced_column_id);
        DataToken token = deleting_row.tokens[idx];

        switch (fk.action)
        {
        case OnDeleteFkAction::RESTRICT:
            if (dql_.value_exists(current_table, col.name, token))
                throw EngineException(
                    "FOREIGN KEY violation: cannot delete from '" + deleted_from_table.name +
                    "': value is referenced by '" + current_table.name + "." + col.name + "'",
                    EngineException::Code::FOREIGN_KEY_VIOLATION);
            break;
        case OnDeleteFkAction::CASCADE:
        {
            auto rows = dql_.get_rows_with_value(current_table, col.name, token);
            for (const auto& child_row : rows)
                on_delete(current_table, child_row, txn);
            dml_.delete_selected(current_table, rows, txn);
            break;
        }
        case OnDeleteFkAction::SET_NULL:
        {
            auto rows = dql_.get_rows_with_value(current_table, col.name, token);
            RowUpdate update = {
                AssignLiteral{col.id, DataToken({}, DataType::_NULL)}
            };
            dml_.update_selected(current_table, update, rows, txn);
            break;
        }
        case OnDeleteFkAction::NO_ACTION:
        default:
            break;
        }
    }

    void
    ConstraintEnforcer::on_delete(
        const MetaTable& mt,
        const DataRow& deleting_row,
        txn::Transaction* txn)
    {
        auto tables = catalog_.get_all_tables();
        for (auto* table : tables)
        {
            for (int i = 0; i < table->columns.size(); ++i)
            {
                const auto& col = table->columns[i];
                if (auto* fk = col.get_constraint<MetaForeignKeyConstraint>())
                {
                    if (fk->referenced_table_id != mt.id)
                        continue;

                    handle_fk_on_delete(
                        mt,
                        *table,
                        *fk,
                        deleting_row,
                        col,
                        txn);
                }
            }
        }
    }
}