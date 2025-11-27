//
// Created by poproshaikin on 26.11.25.
//

#ifndef DELTABASE_STD_DATA_BUFFERS_HPP
#define DELTABASE_STD_DATA_BUFFERS_HPP
#include "data_buffers.hpp"
#include "io_manager.hpp"

namespace storage
{
    class StdDataBuffers final : public IDataBuffers
    {
        IIOManager& io_manager_;
        std::vector<types::DataPage> pages_;

        void
        init();

        types::DataPage&
        create_page(const types::MetaTable& table);

        types::DataPage&
        get_available_page(const types::MetaTable& table, uint64_t size);

    public:
        explicit
        StdDataBuffers(IIOManager& io_manager);

        types::DataPage
        get_page(types::Uuid page_id) override;

        std::vector<types::DataPage>
        get_pages(const types::MetaTable& table) override;

        void
        insert_row(types::MetaTable &table, types::DataRow &row) override;
    };
}

#endif //DELTABASE_STD_DATA_BUFFERS_HPP