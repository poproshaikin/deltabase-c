//
// Created by poproshaikin on 25.11.25.
//

#ifndef DELTABASE_STD_CATALOG_HPP
#define DELTABASE_STD_CATALOG_HPP
#include "catalog.hpp"
#include "../../types/include/catalog_snapshot.hpp"
#include "io_manager.hpp"

namespace storage
{
    class StdCatalog final : public ICatalog
    {
        IIOManager& io_manager_;
        types::CatalogSnapshot actual_snapshot_;
        types::CatalogSnapshot stored_snapshot_;
        std::vector<types::CatalogSnapshot> snapshots_;

        void
        init();

    public:
        explicit
        StdCatalog(IIOManager& io_manager);

        types::CatalogSnapshot
        get_snapshot() override;

        void
        save_snapshot(const types::CatalogSnapshot& snapshot) override;

        void
        flush() override;

        void
        commit_snapshot(const types::CatalogSnapshot& snapshot) override;
    };
}

#endif //DELTABASE_STD_CATALOG_HPP