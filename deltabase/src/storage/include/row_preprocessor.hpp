//
// Created by poproshaikin on 6/19/26.
//

#ifndef DELTABASE_ROW_PREPROCESSOR_HPP
#define DELTABASE_ROW_PREPROCESSOR_HPP

#include "catalog.hpp"
#include "../../types/include/data_token.hpp"
#include "../../types/include/meta_table.hpp"
#include "../../transactions/include/transaction.hpp"

#include <optional>
#include <string>
#include <vector>

namespace storage
{
    class RowPreprocessor
    {
        CatalogCache& catalog_;

    public:
        explicit RowPreprocessor(CatalogCache& catalog);

        void
        prepare_row(
            const types::MetaTable& mt,
            const std::optional<std::vector<std::string>>& cols,
            std::vector<types::DataToken>& row,
            txn::Transaction& txn);
    };
}

#endif //DELTABASE_ROW_PREPROCESSOR_HPP
