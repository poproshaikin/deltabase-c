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

    // A refused reservation must not count towards usage, or the budget stays poisoned
    // for the rest of the query.
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
