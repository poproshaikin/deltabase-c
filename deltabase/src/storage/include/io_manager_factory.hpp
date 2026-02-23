//
// Created by poproshaikin on 08.01.26.
//

#ifndef DELTABASE_IO_MANAGER_FACTORY_HPP
#define DELTABASE_IO_MANAGER_FACTORY_HPP
#include "io_manager.hpp"

namespace storage
{
    class IOManagerFactory
    {
    public:
        std::unique_ptr<IIOManager>
        make_io_manager(const types::Config& config) const;
    };
}

#endif //DELTABASE_IO_MANAGER_FACTORY_HPP