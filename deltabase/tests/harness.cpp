#include "harness.hpp"

#include "static_storage.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <unistd.h>

namespace tst
{
    namespace
    {
        size_t failures_ = 0;
    }

    std::vector<TestCase>&
    registry()
    {
        static std::vector<TestCase> tests;
        return tests;
    }

    Register::Register(const char* suite, const char* name, TestFn fn, bool manual)
    {
        registry().push_back({suite, name, std::move(fn), manual});
    }

    void
    fail(const std::string& what)
    {
        ++failures_;
        std::cout << "      FAIL " << what << "\n";
    }

    void
    check(bool ok, const std::string& what)
    {
        if (!ok)
            fail(what);
    }

    void
    check_eq(const std::string& got, const std::string& want, const std::string& what)
    {
        if (got == want)
            return;

        fail(what);
        std::cout << "           want: " << want << "\n"
                  << "           got : " << got << "\n";
    }

    void
    check_eq(uint64_t got, uint64_t want, const std::string& what)
    {
        if (got == want)
            return;

        fail(what);
        std::cout << "           want: " << want << ", got: " << got << "\n";
    }

    size_t
    current_failures()
    {
        return failures_;
    }

    void
    reset_current_failures()
    {
        failures_ = 0;
    }

    std::string
    render(const types::DataRow& row)
    {
        std::string out;
        for (const auto& token : row.tokens)
        {
            switch (token.type)
            {
            case types::DataType::INTEGER: out += std::to_string(token.as<int>()); break;
            case types::DataType::REAL:    out += std::to_string(token.as<double>()); break;
            case types::DataType::BOOL:    out += token.as<bool>() ? "true" : "false"; break;
            case types::DataType::TEXT:    out += token.as<std::string>(); break;
            case types::DataType::_NULL:   out += "NULL"; break;
            default:                       out += "?"; break;
            }
            out += " ";
        }
        return out;
    }

    TestDb::TestDb(const std::string& prefix, uint64_t work_mem)
    {
        const auto stamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();

        name_ = prefix + "_" + std::to_string(getpid()) + "_" + std::to_string(stamp);

        std::error_code ec;
        std::filesystem::remove_all(
            misc::StaticStorage::get_executable_path() / "data" / name_, ec);

        auto config = types::Config::std(name_);
        if (work_mem != 0)
            config.work_mem = work_mem;

        engine_ = std::make_unique<engine::Engine>();
        engine_->create_db(config);
    }

    TestDb::~TestDb()
    {
        // a moved-from fixture owns nothing
        if (!engine_)
            return;

        engine_.reset();

        std::error_code ec;
        std::filesystem::remove_all(
            misc::StaticStorage::get_executable_path() / "data" / name_, ec);
    }

    void
    TestDb::exec(const std::string& query)
    {
        auto result = engine_->execute_query(query);
        types::DataRow row;
        while (result->next(row))
        {
        }
    }

    std::string
    TestDb::rows(const std::string& query)
    {
        std::string out;
        try
        {
            auto result = engine_->execute_query(query);
            types::DataRow row;
            while (result->next(row))
                out += render(row) + "| ";
        }
        catch (const std::exception& ex)
        {
            return std::string("ERROR: ") + ex.what();
        }
        return out;
    }

    uint64_t
    TestDb::count(const std::string& query)
    {
        auto result = engine_->execute_query(query);
        types::DataRow row;
        uint64_t n = 0;
        while (result->next(row))
            ++n;
        return n;
    }

    std::string
    TestDb::error(const std::string& query)
    {
        try
        {
            auto result = engine_->execute_query(query);
            types::DataRow row;
            while (result->next(row))
            {
            }
        }
        catch (const std::exception& ex)
        {
            return ex.what();
        }
        return "";
    }
}
