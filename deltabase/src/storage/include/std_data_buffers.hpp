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
    public:
        struct Entry
        {
            types::DataPage value;
            uint64_t version;

            explicit
            Entry(const types::DataPage& value) : value(value), version(last_version_++)
            {
            }

        private:
            static inline uint64_t last_version_ = 0;
        };

        struct TableData
        {
            types::Uuid table_id;
            std::vector<Entry> entries;

            explicit
            TableData(
                const types::Uuid& table_id,
                const std::vector<Entry>& entries
            ) : table_id(table_id),
                entries(entries)
            {
            }
        };

    private:
        IIOManager& io_manager_;
        std::vector<TableData> table_data_;

        void
        init();

    public:
        explicit
        StdDataBuffers(IIOManager& io_manager);

        types::DataPage
        get_page(types::Uuid page_id) override;

        std::vector<types::DataPage>
        get_pages(const types::MetaTable& table) override;
    };
}

#endif //DELTABASE_STD_DATA_BUFFERS_HPP