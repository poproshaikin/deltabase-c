//
// The per-query memory budget.
//

#include "harness.hpp"

#include "memory_budget.hpp"

TEST(memory_budget, reserve_and_release)
{
    misc::MemoryBudget budget(100);

    budget.reserve(60);
    budget.reserve(40);

    bool exceeded = false;
    try
    {
        budget.reserve(1);
    }
    catch (const EngineException& ex)
    {
        exceeded = ex.code() == EngineException::Code::MEMORY_BUDGET_EXCEEDED;
    }
    tst::check(exceeded, "reserving past the limit throws MEMORY_BUDGET_EXCEEDED");

    budget.release(40);
    bool reusable = true;
    try
    {
        budget.reserve(40);
    }
    catch (const std::exception&)
    {
        reusable = false;
    }
    tst::check(reusable, "a refused reservation leaves the counter untouched");

    bool over_released = false;
    try
    {
        budget.release(1000);
    }
    catch (const std::logic_error&)
    {
        over_released = true;
    }
    tst::check(over_released, "releasing more than was reserved is an accounting bug");
}

TEST(memory_budget, reset)
{
    misc::MemoryBudget budget(100);
    budget.reserve(100);

    budget.reset(10);

    bool refused = false;
    try
    {
        budget.reserve(11);
    }
    catch (const EngineException&)
    {
        refused = true;
    }
    tst::check(refused, "reset applies the new limit");

    bool allowed = true;
    try
    {
        budget.reserve(10);
    }
    catch (const std::exception&)
    {
        allowed = false;
    }
    tst::check(allowed, "and zeroes the usage");
}

TEST(memory_budget, a_query_is_refused_rather_than_exhausting_memory)
{
    // Room for roughly sixteen rows.
    tst::TestDb db("budget", 2000);
    db.exec("create table common.t(id integer);");
    for (int i = 0; i < 200; ++i)
        db.exec("insert into common.t (id) values (" + std::to_string(i) + ");");

    const auto message = db.error("select * from common.t");
    tst::check(message.find("memory budget") != std::string::npos,
               "materialising 200 rows exceeds the budget: " + message);

    // Refusal has to follow actual consumption, not the size of the table.
    tst::check_eq(db.count("select * from common.t limit 3"), 3, "a small result still runs");
    tst::check_eq(db.count("select * from common.t where id >= 199"), 1,
                  "a narrow filter still runs");
}

TEST(memory_budget, a_sort_accounts_for_the_rows_it_holds)
{
    tst::TestDb db("budget_sort", 2000);
    db.exec("create table common.t(id integer);");
    for (int i = 0; i < 200; ++i)
        db.exec("insert into common.t (id) values (" + std::to_string(i) + ");");

    const auto message = db.error("select id from common.t order by id");
    tst::check(message.find("memory budget") != std::string::npos,
               "an unbounded sort over 200 rows is refused: " + message);

    tst::check_eq(db.count("select id from common.t order by id limit 3"), 3,
                  "a bounded sort fits");
    tst::check_eq(db.rows("select id from common.t order by id limit 3"), "0 | 1 | 2 | ",
                  "and still returns the right rows");
    tst::check_eq(db.count("select id from common.t order by id desc limit 2 offset 1"), 2,
                  "a bounded sort with an offset fits too");
}

TEST(memory_budget, serving_a_row_releases_its_reservation)
{
    tst::TestDb db("budget_handoff", 40000);
    db.exec("create table common.t(id integer);");
    for (int i = 0; i < 200; ++i)
        db.exec("insert into common.t (id) values (" + std::to_string(i) + ");");

    // Fits twice over, but not four times: a missing release would charge every row twice.
    tst::check_eq(db.count("select id from common.t order by id"), 200,
                  "a sort whose output is materialised is not charged twice");
}

// The planner only picks an index scan when it estimates few rows, so the amount is small.
TEST(memory_budget, an_index_scan_accounts_for_its_table)
{
    tst::TestDb db("budget_index", 20000);
    db.exec("create table common.t(id integer primary key, v integer);");
    for (int i = 0; i < 200; ++i)
        db.exec("insert into common.t (id, v) values (" + std::to_string(i) + ", " +
                std::to_string(i % 10) + ");");

    tst::check_eq(db.rows("select id from common.t where id == 42"), "42 | ", "index lookup");
    tst::check_eq(db.count("select id from common.t where id == 199"), 1, "last key");
    tst::check_eq(db.count("select id from common.t where id == 500"), 0, "absent key");

    const auto message = db.error("select id, v from common.t");
    tst::check(message.find("memory budget") != std::string::npos,
               "a full result is still refused: " + message);
}

TEST(memory_budget, default_limit_is_generous)
{
    tst::TestDb db("budget_default");
    db.exec("create table common.t(id integer, name text);");
    for (int i = 0; i < 500; ++i)
        db.exec("insert into common.t (id, name) values (" + std::to_string(i) +
                ", 'row number " + std::to_string(i) + "');");

    tst::check_eq(db.count("select * from common.t"), 500, "plain scan");
    tst::check_eq(db.count("select * from common.t order by id"), 500, "sorted");
    tst::check_eq(db.count("select * from common.t order by id limit 5"), 5, "bounded sort");
}
