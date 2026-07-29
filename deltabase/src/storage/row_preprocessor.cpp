//
// Created by poproshaikin on 6/19/26.
//

#include "row_preprocessor.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace storage
{
    using namespace types;

    RowPreprocessor::RowPreprocessor(CatalogCache & catalog, IIOManager & io_manager)
        : catalog_(catalog), io_manager_(io_manager)
    {
    }

    void
    RowPreprocessor::prepare_row(
        const MetaTable & mt,
        std::optional<std::vector<std::string>> & cols,
        std::vector<DataToken> & row,
        txn::Transaction & txn)
    {
        const DataToken null_token(Bytes{}, DataType::_NULL);

        // Step 1: map named columns to positional, fill gaps with NULL
        std::vector<DataToken> normalized(mt.columns.size(), null_token);

        if (!cols.has_value())
        {
            for (size_t i = 0; i < row.size() && i < mt.columns.size(); ++i)
                normalized[i] = row[i];
        }
        else
        {
            for (size_t i = 0; i < cols->size(); ++i)
            {
                int64_t col_idx = mt.get_column_idx((*cols)[i]);
                if (col_idx != -1 && i < row.size())
                    normalized[static_cast<size_t>(col_idx)] = row[i];
            }
        }

        // Step 2: apply DEFAULT for missing (NULL) positions
        for (size_t i = 0; i < mt.columns.size(); ++i)
        {
            if (normalized[i].type == DataType::_NULL)
            {
                const auto* dc = mt.columns[i].get_constraint<MetaDefaultConstraint>();
                if (dc)
                    normalized[i] = dc->value;
            }
        }

        row = std::move(normalized);
        cols = std::nullopt;

        // Step 3: AUTOINCREMENT — generate or sync sequence
        for (size_t i = 0; i < mt.columns.size(); ++i)
        {
            const auto& column = mt.columns[i];
            const auto* ai = column.get_constraint<MetaAutoIncrementConstraint>();
            if (!ai)
                continue;

            auto* seq = catalog_.get_sequence(ai->sequence_id);
            if (!seq)
                throw std::runtime_error("Sequence for autoincrement column not found");

            if (row[i].type != DataType::_NULL)
            {
                // Caller provided an explicit value — sync sequence if needed
                int32_t provided_val = 0;
                std::memcpy(&provided_val, row[i].bytes.data(), sizeof(int32_t));

                if (provided_val >= seq->current_value)
                {
                    const MetaSequence before = *seq;
                    seq->current_value = provided_val;
                    UpdateSequenceRecord seq_record(before, *seq);
                    txn.append_log(seq_record);
                    io_manager_.write_seq(*seq);
                }
                continue;
            }

            const MetaSequence before = *seq;
            const int new_val = ++seq->current_value;

            Bytes val_bytes(sizeof(int));
            std::memcpy(val_bytes.data(), &new_val, sizeof(int));
            row[i] = DataToken(val_bytes, DataType::INTEGER);

            UpdateSequenceRecord seq_record(before, *seq);
            txn.append_log(seq_record);
            io_manager_.write_seq(*seq);
        }
    }
}
