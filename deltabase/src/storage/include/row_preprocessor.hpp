//
// Created by poproshaikin on 6/19/26.
//

#ifndef DELTABASE_ROW_PREPROCESSOR_HPP
#define DELTABASE_ROW_PREPROCESSOR_HPP

#include "catalog.hpp"
#include "io_manager.hpp"
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
        IIOManager& io_manager_;

    public:
        explicit RowPreprocessor(CatalogCache& catalog, IIOManager& io_manager);

        void
        prepare_row(
            const types::MetaTable& mt,
            std::optional<std::vector<std::string>>& cols,
            std::vector<types::DataToken>& row,
            txn::Transaction& txn);
    };
}

#endif //DELTABASE_ROW_PREPROCESSOR_HPP
