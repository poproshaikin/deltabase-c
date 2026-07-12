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
        for (size_t i = 0; i < mt.columns.size(); ++i)
        {
            const auto& column = mt.columns[i];
            const auto* ai = column.get_constraint<MetaAutoIncrementConstraint>();
            if (!ai)
                continue;

            bool caller_provided = false;
            if (cols.has_value())
            {
                for (const auto& col_name : *cols)
                    if (col_name == column.name)
                    {
                        caller_provided = true;
                        break;
                    }
            }
            else
            {
                caller_provided = (i < row.size() && row[i].type != DataType::_NULL);
            }

            if (caller_provided)
            {
                if (!cols.has_value())
                    continue;

                auto it = std::ranges::find(*cols, column.name);
                if (it != cols->end())
                {
                    const size_t idx = static_cast<size_t>(std::distance(cols->begin(), it));
                    if (idx < row.size() && row[idx].type == DataType::INTEGER)
                    {
                        int32_t provided_val = 0;
                        std::memcpy(&provided_val, row[idx].bytes.data(), sizeof(int32_t));

                        auto* seq = catalog_.get_sequence(ai->sequence_id);
                        if (seq && provided_val >= seq->current_value)
                        {
                            const MetaSequence before = *seq;
                            seq->current_value = provided_val;

                            UpdateSequenceRecord seq_record(before, *seq);
                            txn.append_log(seq_record);
                            io_manager_.write_seq(*seq);
                        }
                    }
                }
            }

            auto* seq = catalog_.get_sequence(ai->sequence_id);
            if (!seq)
                throw std::runtime_error("Sequence for autoincrement column not found");

            const MetaSequence before = *seq;
            const int new_val = ++seq->current_value;

            Bytes val_bytes(sizeof(int));
            std::memcpy(val_bytes.data(), &new_val, sizeof(int));
            DataToken token(val_bytes, DataType::INTEGER);

            if (cols.has_value())
            {
                cols->push_back(column.name);
                row.push_back(token);
            }
            else
            {
                if (row.size() <= i)
                    row.resize(i + 1, DataToken(Bytes{}, DataType::_NULL));
                row[i] = token;
            }

            UpdateSequenceRecord seq_record(before, *seq);
            txn.append_log(seq_record);
            io_manager_.write_seq(*seq);
        }
    }
}
