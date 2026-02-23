//
// Created by poproshaikin on 08.01.26.
//

#include "generic_query_validator.hpp"
#include "../../storage/include/io_manager_factory.hpp"

#include <iostream>

namespace exq
{
    using namespace storage;

    GenericQueryValidator::GenericQueryValidator(const types::Config& cfg) : cfg_(cfg)
    {
        IOManagerFactory factory;
        io_manager_ = factory.make_io_manager(cfg);
    }

    bool
    GenericQueryValidator::exists_db(const std::string& db_name) const
    {
        return io_manager_->exists_db(db_name);
    }
}