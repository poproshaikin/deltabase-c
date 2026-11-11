//
// DataToken::compare_ordered -- the total order used by sorting and by index keys.
//

#include "harness.hpp"

#include "data_token.hpp"

#include <algorithm>
#include <cstring>
#include <random>
#include <vector>

using namespace types;

namespace
{
    DataToken
    integer(int value)
    {
        Bytes bytes(sizeof(int));
        std::memcpy(bytes.data(), &value, sizeof(int));
        return DataToken(bytes, DataType::INTEGER);
    }

    DataToken
    real(double value)
    {
        Bytes bytes(sizeof(double));
        std::memcpy(bytes.data(), &value, sizeof(double));
        return DataToken(bytes, DataType::REAL);
    }

    DataToken
    text(const std::string& value)
    {
        return DataToken(Bytes(value.begin(), value.end()), DataType::TEXT);
    }

    DataToken
    boolean(bool value)
    {
        Bytes bytes(1);
        bytes[0] = value ? 1 : 0;
        return DataToken(bytes, DataType::BOOL);
    }

    DataToken
    null()
    {
        return DataToken(Bytes{}, DataType::_NULL);
    }

    bool
    ordered_less(const DataToken& left, const DataToken& right)
    {
        return DataToken::compare_ordered(left, right) < 0;
    }

    // std::sort requires a strict weak ordering. The requirement that trips over NULL is
    // transitivity of the induced equivalence: comparing every triple is the only way to
    // see it, and violating it is undefined behaviour rather than a wrong order.
    template <typename Less>
    void
    check_strict_weak_ordering(const std::vector<DataToken>& tokens, Less less,
                               const std::string& label)
    {
        size_t irreflexive = 0;
        size_t asymmetric = 0;
        size_t transitive = 0;
        size_t equivalence = 0;

        for (const auto& a : tokens)
            if (less(a, a))
                ++irreflexive;

        for (const auto& a : tokens)
            for (const auto& b : tokens)
                if (less(a, b) && less(b, a))
                    ++asymmetric;

        for (const auto& a : tokens)
            for (const auto& b : tokens)
                for (const auto& c : tokens)
                {
                    if (less(a, b) && less(b, c) && !less(a, c))
                        ++transitive;

                    const bool ab = !less(a, b) && !less(b, a);
                    const bool bc = !less(b, c) && !less(c, b);
                    const bool ac = !less(a, c) && !less(c, a);
                    if (ab && bc && !ac)
                        ++equivalence;
                }

        tst::check_eq(irreflexive, 0, label + ": irreflexivity");
        tst::check_eq(asymmetric, 0, label + ": asymmetry");
        tst::check_eq(transitive, 0, label + ": transitivity");
        tst::check_eq(equivalence, 0, label + ": transitivity of equivalence");
    }
}

TEST(data_token, compare_ordered_is_a_strict_weak_ordering)
{
    const std::vector<DataToken> numeric = {
        integer(5), integer(7), null(), integer(-3), integer(0),
        real(5.0), real(1.5), null(), integer(5),
    };
    check_strict_weak_ordering(numeric, ordered_less, "numeric and NULL");

    const std::vector<DataToken> mixed = {
        integer(1), text("a"), boolean(true), null(), real(2.5), text("b"), boolean(false),
    };
    check_strict_weak_ordering(mixed, ordered_less, "every type and NULL");
}

// The SQL comparison operators are deliberately NOT a valid sort predicate: NULL compares
// UNKNOWN against everything, which makes it equivalent to every value at once.
TEST(data_token, sql_operators_are_not_a_sort_predicate)
{
    const auto sql_less = [](const DataToken& a, const DataToken& b) { return a < b; };

    size_t violations = 0;
    const std::vector<DataToken> tokens = {null(), integer(5), integer(7)};

    for (const auto& a : tokens)
        for (const auto& b : tokens)
            for (const auto& c : tokens)
            {
                const bool ab = !sql_less(a, b) && !sql_less(b, a);
                const bool bc = !sql_less(b, c) && !sql_less(c, b);
                const bool ac = !sql_less(a, c) && !sql_less(c, a);
                if (ab && bc && !ac)
                    ++violations;
            }

    tst::check(violations > 0,
               "operator< must still violate the contract -- otherwise this test proves nothing");
}

TEST(data_token, null_ordering)
{
    tst::check_eq(static_cast<uint64_t>(DataToken::compare_ordered(null(), null()) == 0), 1,
                  "NULL equals NULL");
    tst::check_eq(static_cast<uint64_t>(DataToken::compare_ordered(null(), integer(5)) > 0), 1,
                  "NULL sorts after a value");
    tst::check_eq(static_cast<uint64_t>(DataToken::compare_ordered(integer(5), null()) < 0), 1,
                  "a value sorts before NULL");
}

TEST(data_token, numeric_widening)
{
    tst::check_eq(static_cast<uint64_t>(DataToken::compare_ordered(integer(5), real(5.0)) == 0), 1,
                  "INTEGER 5 equals REAL 5.0");
    tst::check_eq(static_cast<uint64_t>(DataToken::compare_ordered(integer(5), real(5.5)) < 0), 1,
                  "INTEGER 5 is before REAL 5.5");
}

TEST(data_token, text_is_ordered_by_unsigned_bytes)
{
    std::vector<std::string> values = {
        "apple", "Banana", "app", "banana", "Apple", "", "zebra", "\xC3\xA4pfel", "APPLE2",
    };

    std::sort(values.begin(), values.end(),
              [](const std::string& a, const std::string& b)
              { return DataToken::compare_ordered(text(a), text(b)) < 0; });

    std::string joined;
    for (const auto& value : values)
        joined += "'" + value + "' ";

    // Case-sensitive, uppercase before lowercase, shorter prefix first, UTF-8 by code point.
    tst::check_eq(joined,
                  "'' 'APPLE2' 'Apple' 'Banana' 'app' 'apple' 'banana' 'zebra' '\xC3\xA4pfel' ",
                  "byte-lexicographic order");
}

TEST(data_token, sorting_with_nulls_is_safe_and_puts_them_last)
{
    // Large enough to reach introsort's quicksort partitioning, where a comparator that
    // breaks the contract walks off the buffer instead of merely ordering badly.
    std::vector<DataToken> tokens;
    std::mt19937 rng(42);
    for (int i = 0; i < 4000; ++i)
    {
        const bool is_null = rng() % 10 == 0;
        tokens.push_back(is_null ? null() : integer(static_cast<int>(rng() % 500) - 250));
    }

    std::stable_sort(tokens.begin(), tokens.end(), ordered_less);

    size_t first_null = tokens.size();
    size_t nulls = 0;
    for (size_t i = 0; i < tokens.size(); ++i)
        if (tokens[i].type == DataType::_NULL)
        {
            if (first_null == tokens.size())
                first_null = i;
            ++nulls;
        }

    bool monotonic = true;
    for (size_t i = 1; i < first_null; ++i)
        if (DataToken::compare_ordered(tokens[i - 1], tokens[i]) > 0)
            monotonic = false;

    tst::check(monotonic, "the non-NULL prefix is ascending");
    tst::check_eq(first_null + nulls, tokens.size(), "all NULLs end up last");
}
