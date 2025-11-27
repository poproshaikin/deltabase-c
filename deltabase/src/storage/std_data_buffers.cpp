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
        for (auto data = io_manager_.load_tables_data(); const auto& [fst, snd] : data)
        {
            const auto& table_id = fst;
            const auto& pages = snd;

            pages_.emplace_back(table_id, pages);
        }
    }

    DataPage&
    StdDataBuffers::create_page(const MetaTable& table)
    {
        DataPage page;
        page.header.table_id = table.id;

        pages_.push_back(std::move(page));
        return pages_.back();
    }

    DataPage&
    StdDataBuffers::get_available_page(const MetaTable& table, uint64_t size)
    {
        for (auto& page : pages_)
            if (page.header.table_id == table.id && page.header.size + size <= DataPage::MAX_SIZE)
                return page;

        return create_page(table);
    }

    DataPage
    StdDataBuffers::get_page(Uuid page_id)
    {
        for (const auto& page : pages_)
            if (page.header.table_id == page_id)
                return page;

        throw std::runtime_error(
            "StdDataBuffers::get_page: page " + page_id.to_string() + " not found");
    }

    std::vector<DataPage>
    StdDataBuffers::get_pages(const MetaTable& table)
    {
        std::vector<DataPage> pages;

        for (const auto& page : pages_)
            if (table.id == page.header.table_id)
                pages.push_back(page);

        return pages;
    }

    void
    StdDataBuffers::insert_row(MetaTable& table, DataRow& row)
    {
        uint64_t size = row.estimate_size();

        auto& page = get_available_page(table, size);
        page.version += DataPage::last_version_++;
        page.header.size += row.estimate_size();
        page.header.max_rid += row.row_id += table.last_rid++;
        page.rows.push_back(row);
    }
}