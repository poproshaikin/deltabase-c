#pragma once 

#include "action.hpp"

namespace exe {
    struct SingleActionPlan {
        Action action;
    };

    struct TransactionPlan {
        std::vector<Action> actions;
        // TransactionOptions options
    };

    using QueryPlan = std::variant<
        SingleActionPlan,
        TransactionPlan
    >;
}