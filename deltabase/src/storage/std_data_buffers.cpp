//
// Created by poproshaikin on 26.11.25.
//

#include "std_data_buffers.hpp"

#include <iostream>

namespace storage
{
    using namespace types;

    StdDataBuffers::StdDataBuffers(IIOManager& io_manager) : io_manager_(io_manager)
    {
        init();
    }

    void
    StdDataBuffers::init()
    {
        std::vector<std::pair<Uuid, std::vector<DataPage> > > data = io_manager_.load_tables_data();

        for (const auto& entry : data)
        {
            Uuid table_id = entry.first;
            const std::vector<DataPage>& pages = entry.second;

            std::vector<Entry> entries;
            entries.reserve(pages.size());

            for (const auto& page : pages)
            {
                entries.emplace_back(Entry(page));
            }

            table_data_.emplace_back(table_id, entries);
        }
    }

    StdDataBuffers::Entry
    StdDataBuffers::get_page(Uuid page_id)
    {
        for (const auto& table_entry : table_data_)
            for (auto& page : table_entry.entries)
                if (page.value.header.id == page_id)
                    return page.value;

        throw std::runtime_error(
            "StdDataBuffers::get_page: page " + page_id.to_string() + " not found");
    }

    std::vector<DataPage>
    StdDataBuffers::get_pages(const MetaTable& table)
    {
        for (const auto& table_entry : table_data_)
        {
            if (table.id == table_entry.table_id)
                return table_entry.entries;
        }
    }
}