//
// Created by poproshaikin on 24.11.25.
//

#ifndef DELTABASE_DATA_BUFFERS_HPP
#define DELTABASE_DATA_BUFFERS_HPP
#include "std_data_buffers.hpp"
#include "../../types/include/uuid.hpp"
#include "../../types/include/data_page.hpp"


namespace storage
{
    class IDataBuffers
    {
    public:
        virtual ~IDataBuffers() = default;

        virtual types::DataPage
        get_page(types::Uuid uuid) = 0;

        virtual std::vector<types::DataPage>
        get_pages(const types::MetaTable& table) = 0;
    };
}

#endif //DELTABASE_DATA_BUFFERS_HPP