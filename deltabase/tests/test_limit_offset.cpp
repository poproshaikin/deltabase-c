//
// LIMIT / OFFSET, and the bounded-heap path the planner takes when a LIMIT is present.
//

#include "harness.hpp"

#include <algorithm>
#include <random>
#include <vector>

namespace
{
    tst::TestDb
    make_db()
    {
        tst::TestDb db("limit_offset");
        db.exec("create table common.t(id integer, name text);");

        const int ids[] = {3, 1, 5, 2, 4};
        for (const int id : ids)
            db.exec("insert into common.t (id, name) values (" + std::to_string(id) +
                    ", 'n" + std::to_string(id) + "');");

        db.exec("insert into common.t (name) values ('nullid');");
        return db;
    }
}

TEST(limit, basics)
{
    auto db = make_db();

    tst::check_eq(db.rows("select id from common.t order by id limit 3"),
                  "1 | 2 | 3 | ", "the first three in order");
    tst::check_eq(db.rows("select id from common.t order by id desc limit 3"),
                  "NULL | 5 | 4 | ", "descending, NULL first");
    tst::check_eq(db.rows("select id from common.t order by id limit 0"),
                  "", "limit zero returns nothing");
    tst::check_eq(db.rows("select id from common.t order by id limit 99"),
                  "1 | 2 | 3 | 4 | 5 | NULL | ", "a limit past the end returns everything");
}

TEST(limit, offset)
{
    auto db = make_db();

    tst::check_eq(db.rows("select id from common.t order by id offset 2"),
                  "3 | 4 | 5 | NULL | ", "offset without limit");
    tst::check_eq(db.rows("select id from common.t order by id offset 99"),
                  "", "an offset past the end returns nothing");
    tst::check_eq(db.rows("select id from common.t order by id limit 2 offset 1"),
                  "2 | 3 | ", "offset applies before limit");
    tst::check_eq(db.rows("select id from common.t order by id limit 0 offset 2"),
                  "", "limit zero wins over offset");
    tst::check_eq(db.rows("select id from common.t order by id limit 99 offset 4"),
                  "5 | NULL | ", "limit past the end, with an offset");
    tst::check_eq(db.rows("select id from common.t order by id desc limit 2 offset 1"),
                  "5 | 4 | ", "descending with both");
}

// Applying the limit before the sort is the classic way to get this wrong: it would return
// three arbitrary rows in order rather than the three first rows.
TEST(limit, applies_after_the_sort)
{
    auto db = make_db();

    tst::check_eq(db.rows("select id from common.t order by id limit 3"),
                  "1 | 2 | 3 | ", "ascending");
    tst::check_eq(db.rows("select id from common.t where id >= 2 order by id limit 2"),
                  "2 | 3 | ", "with a filter underneath");
}

TEST(limit, without_order_by)
{
    auto db = make_db();

    // No ORDER BY, so this is insertion order: 3 1 5 2 4 NULL.
    tst::check_eq(db.rows("select id from common.t limit 2 offset 1"),
                  "1 | 5 | ", "scan order, skipping one");
}

// With a LIMIT the sort keeps only limit+offset rows in a heap instead of materialising
// everything. The result must be identical to the full sort either way.
TEST(limit, bounded_sort_agrees_with_the_full_sort)
{
    tst::TestDb db("limit_scale");
    db.exec("create table common.t(id integer);");

    constexpr int rows = 1500;
    std::vector<int> values(rows);
    for (int i = 0; i < rows; ++i)
        values[i] = i;

    std::mt19937 rng(7);
    std::shuffle(values.begin(), values.end(), rng);
    for (const int value : values)
        db.exec("insert into common.t (id) values (" + std::to_string(value) + ");");

    const auto full = db.rows("select id from common.t order by id");

    auto prefix = [&](const std::string& all, int count)
    {
        std::string out;
        int seen = 0;
        size_t pos = 0;
        while (seen < count)
        {
            const size_t next = all.find("| ", pos);
            out += all.substr(pos, next + 2 - pos);
            pos = next + 2;
            ++seen;
        }
        return out;
    };

    tst::check_eq(db.rows("select id from common.t order by id limit 10"), prefix(full, 10),
                  "limit 10 matches the first ten of the full sort");
    tst::check_eq(db.count("select id from common.t order by id limit 10 offset 1495"), 5,
                  "an offset near the end returns the remainder");
    tst::check_eq(db.rows("select id from common.t order by id desc limit 3"),
                  "1499 | 1498 | 1497 | ", "descending top three");
    tst::check_eq(db.count("select id from common.t order by id"), rows,
                  "the unbounded sort still returns everything");
}

TEST(limit, ties_under_a_bound)
{
    tst::TestDb db("limit_ties");
    db.exec("create table common.t(id integer, grp integer);");

    const int ids[] = {3, 1, 5, 2, 4};
    for (const int id : ids)
        db.exec("insert into common.t (id, grp) values (" + std::to_string(id) + ", 7);");

    // Every key is equal, so the bounded path must keep the same rows the full sort would.
    tst::check_eq(db.rows("select id from common.t order by grp limit 3"),
                  "3 | 1 | 5 | ", "bounded sort keeps the first rows on a full tie");
    tst::check_eq(db.rows("select id from common.t order by grp desc limit 3"),
                  "3 | 1 | 5 | ", "and does so regardless of direction");
}
