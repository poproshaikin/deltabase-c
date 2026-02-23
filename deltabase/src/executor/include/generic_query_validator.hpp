//
// Created by poproshaikin on 08.01.26.
//

#ifndef DELTABASE_GENERIC_QUERY_VALIDATOR_HPP
#define DELTABASE_GENERIC_QUERY_VALIDATOR_HPP
#include "config.hpp"
#include "../../storage/include/io_manager.hpp"

namespace exq
{
    class GenericQueryValidator
    {
        types::Config cfg_;
        std::unique_ptr<storage::IIOManager> io_manager_;

    public:
        explicit
        GenericQueryValidator(const types::Config& cfg);

        bool
        exists_db(const std::string& db_name) const;
    };
}

#endif //DELTABASE_GENERIC_QUERY_VALIDATOR_HPP