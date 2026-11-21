//
// Behaviour when the buffer pool is too small to hold a whole table.
//
// A pool of two pages makes eviction -- and therefore page invalidation -- constant. At the
// default of 10000 pages a table has to exceed ~320 MB before any of this shows up.
//
#include "harness.hpp"

namespace
{
    // Two pages of capacity, and enough rows to need far more.
    tst::TestDb
    make_db(const std::string& prefix, const std::string& ddl, int rows)
    {
        tst::TestDb db(prefix, 0, 2);
        db.exec(ddl);

        for (int i = 0; i < rows; ++i)
            db.exec("insert into common.t (id, name) values (" + std::to_string(i) +
                    ", 'row " + std::to_string(i) + "');");

        return db;
    }
}

TEST(page_cache, a_scan_returns_every_row)
{
    auto db = make_db("cache_scan", "create table common.t(id integer, name text);", 1000);

    tst::check_eq(db.count("select * from common.t"), 1000, "no rows are lost");
    tst::check_eq(db.count("select id from common.t"), 1000, "and none with a projection");
    tst::check_eq(db.count("select id from common.t where id >= 500"), 500, "nor with a filter");
}

TEST(page_cache, a_sort_sees_the_whole_table)
{
    auto db = make_db("cache_sort", "create table common.t(id integer, name text);", 1000);

    tst::check_eq(db.count("select id from common.t order by id"), 1000, "every row is sorted");
    tst::check_eq(db.rows("select id from common.t order by id desc limit 3"),
                  "999 | 998 | 997 | ", "no repeated rows at the top");
    tst::check_eq(db.rows("select id from common.t order by id limit 3"),
                  "0 | 1 | 2 | ", "nor at the bottom");
}

TEST(page_cache, an_indexed_table_is_intact)
{
    auto db = make_db("cache_index",
                      "create table common.t(id integer primary key, name text);", 1000);

    tst::check_eq(db.count("select * from common.t"), 1000, "every row is readable");
    tst::check_eq(db.rows("select id from common.t where id == 500"), "500 | ", "index lookup");
    tst::check_eq(db.count("select id from common.t order by id"), 1000, "sorted scan");
}

TEST(page_cache, update_and_delete_find_their_rows)
{
    auto db = make_db("cache_dml", "create table common.t(id integer, name text);", 500);

    db.exec("update common.t set name = 'touched' where id == 499;");
    tst::check_eq(db.rows("select name from common.t where id == 499"), "touched | ",
                  "a row on a later page is updated");

    db.exec("delete from common.t where id == 0;");
    tst::check_eq(db.count("select * from common.t"), 499, "and one on an earlier page deleted");
    tst::check_eq(db.count("select * from common.t where id == 0"), 0, "the right row went");
}

// Needs the source and destination page at once -- the worst case for a two-page pool.
TEST(page_cache, add_column_rewrites_every_row)
{
    auto db = make_db("cache_alter", "create table common.t(id integer, name text);", 300);

    db.exec("alter table common.t add column extra integer;");

    tst::check_eq(db.count("select * from common.t"), 300, "no row is lost or duplicated");
    tst::check_eq(db.rows("select id, extra from common.t order by id limit 2"),
                  "0 NULL | 1 NULL | ", "the new column reads as NULL");
    tst::check_eq(db.count("select id from common.t order by id"), 300, "and the table sorts");
}

TEST(page_cache, an_index_can_be_built_over_many_pages)
{
    auto db = make_db("cache_build_index", "create table common.t(id integer, name text);", 500);

    db.exec("create index idx on common.t(id);");

    tst::check_eq(db.rows("select id from common.t where id == 499"), "499 | ",
                  "a key from the last page is found");
    tst::check_eq(db.rows("select id from common.t where id == 0"), "0 | ",
                  "and one from the first");
    tst::check_eq(db.count("select * from common.t"), 500, "the table is still whole");
}
