//
// Created by poproshaikin on 24.11.25.
//

#ifndef DELTABASE_CATALOG_HPP
#define DELTABASE_CATALOG_HPP
#include "../../types/include/catalog_snapshot.hpp"

namespace storage
{
    class ICatalog
    {
    public:
        virtual ~ICatalog() = default;

        virtual types::CatalogSnapshot
        get_snapshot() = 0;

        virtual void
        save_snapshot(const types::CatalogSnapshot& snapshot) = 0;

        virtual void
        flush() = 0;

        virtual void
        commit_snapshot(const types::CatalogSnapshot& snapshot) = 0;
    };
}

#endif //DELTABASE_CATALOG_HPP