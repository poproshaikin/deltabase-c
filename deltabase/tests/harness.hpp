//
// Minimal test harness: tests register themselves, the runner in main.cpp runs them.
//

#ifndef DELTABASE_TESTS_HARNESS_HPP
#define DELTABASE_TESTS_HARNESS_HPP

#include "engine.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace tst
{
    using TestFn = std::function<void()>;

    struct TestCase
    {
        std::string suite;
        std::string name;
        TestFn fn;
        bool manual;   // excluded from the default run
    };

    std::vector<TestCase>&
    registry();

    struct Register
    {
        Register(const char* suite, const char* name, TestFn fn, bool manual = false);
    };

// Each test is a free function registered at static init time.
#define TEST(suite, name)                                                         \
    static void suite##_##name();                                                 \
    static const ::tst::Register reg_##suite##_##name(#suite, #name,              \
                                                      suite##_##name);            \
    static void suite##_##name()

// Same, but only runs when named explicitly on the command line.
#define MANUAL_TEST(suite, name)                                                  \
    static void suite##_##name();                                                 \
    static const ::tst::Register reg_##suite##_##name(#suite, #name,              \
                                                      suite##_##name, true);      \
    static void suite##_##name()

    // Reports a failed expectation and keeps going, so one test can check many cases.
    void
    fail(const std::string& what);

    void
    check(bool ok, const std::string& what);

    void
    check_eq(const std::string& got, const std::string& want, const std::string& what);

    void
    check_eq(uint64_t got, uint64_t want, const std::string& what);

    // Number of failures recorded by the test currently running.
    size_t
    current_failures();

    void
    reset_current_failures();

    // A database created fresh for one test and removed on destruction.
    class TestDb
    {
        std::string name_;
        // held indirectly so the destructor can shut the engine down before deleting the
        // directory underneath it -- the other order throws out of a destructor
        std::unique_ptr<engine::Engine> engine_;

    public:
        explicit
        TestDb(const std::string& prefix, uint64_t work_mem = 0);

        ~TestDb();

        TestDb(const TestDb&) = delete;
        TestDb& operator=(const TestDb&) = delete;

        // so a test can build a fixture in a helper and return it
        TestDb(TestDb&&) noexcept = default;

        engine::Engine&
        engine() { return *engine_; }

        const std::string&
        name() const { return name_; }

        void
        exec(const std::string& query);

        // Rows of a query rendered as "v1 v2 | v3 v4 | ", for comparing against a literal.
        std::string
        rows(const std::string& query);

        uint64_t
        count(const std::string& query);

        // The error message a query throws, or "" when it succeeds.
        std::string
        error(const std::string& query);
    };

    std::string
    render(const types::DataRow& row);
}

#endif // DELTABASE_TESTS_HARNESS_HPP
