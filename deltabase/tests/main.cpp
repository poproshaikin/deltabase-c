//
// Test runner. Every test registers itself through the TEST macro in harness.hpp; this just
// walks the registry. With no arguments it runs everything except the manual tests, and with
// arguments it runs the suites or tests whose name contains one of them.
//

#include "harness.hpp"

#include "static_storage.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

int
main(int argc, char** argv)
{
    misc::StaticStorage::set_executable_path(std::filesystem::absolute(argv[0]).parent_path());

    const std::vector<std::string> filters(argv + 1, argv + argc);

    size_t passed = 0;
    size_t failed = 0;
    size_t skipped = 0;
    std::vector<std::string> failures;

    for (const auto& test : tst::registry())
    {
        const std::string full = test.suite + "." + test.name;

        bool selected = filters.empty() ? !test.manual : false;
        for (const auto& filter : filters)
            if (full.find(filter) != std::string::npos)
                selected = true;

        if (!selected)
        {
            ++skipped;
            continue;
        }

        std::cout << "  " << full << std::endl;

        tst::reset_current_failures();
        std::string thrown;

        try
        {
            test.fn();
        }
        catch (const std::exception& ex)
        {
            thrown = ex.what();
        }
        catch (...)
        {
            thrown = "unknown exception";
        }

        if (!thrown.empty())
            std::cout << "      FAIL threw: " << thrown << std::endl;

        if (tst::current_failures() == 0 && thrown.empty())
        {
            ++passed;
        }
        else
        {
            ++failed;
            failures.push_back(full);
        }
    }

    std::cout << "\n" << passed << " passed, " << failed << " failed";
    if (skipped != 0)
        std::cout << ", " << skipped << " not selected";
    std::cout << std::endl;

    for (const auto& name : failures)
        std::cout << "  failed: " << name << std::endl;

    return failed == 0 ? 0 : 1;
}
