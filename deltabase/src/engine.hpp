#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <variant>
#include <memory>
#include <string>

extern "C" {
    #include "core/include/data.h"
}

struct ExecutionResult {
    using Result = std::variant<std::unique_ptr<DataTable>, int>;

    Result result;
    long execution_time_ms;

    ExecutionResult(Result&& result, long execution_time_ms) : 
        result(std::move(result)), execution_time_ms(execution_time_ms) { }
};

class DltEngine {
    public: 
        std::string db_name;
        
        DltEngine(std::string db_name);
        ExecutionResult run(const std::string& sql);
};

#endif