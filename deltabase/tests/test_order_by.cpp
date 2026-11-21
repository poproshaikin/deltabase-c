//
// ORDER BY end to end.
//

#include "harness.hpp"

namespace
{
    // grp is equal in every row, so ordering by it exercises only the row-id tiebreaker.
    tst::TestDb
    make_db()
    {
        tst::TestDb db("order_by");
        db.exec("create table common.t(id integer, grp integer, name text);");

        const int ids[] = {3, 1, 5, 2, 4};
        for (const int id : ids)
            db.exec("insert into common.t (id, grp, name) values (" + std::to_string(id) +
                    ", 7, 'n" + std::to_string(id) + "');");

        db.exec("insert into common.t (grp, name) values (7, 'nullid');");
        return db;
    }
}

TEST(order_by, ascending_and_descending)
{
    auto db = make_db();

    tst::check_eq(db.rows("select id from common.t order by id"),
                  "1 | 2 | 3 | 4 | 5 | NULL | ", "ascending puts NULL last");
    tst::check_eq(db.rows("select id from common.t order by id desc"),
                  "NULL | 5 | 4 | 3 | 2 | 1 | ", "descending puts NULL first");
}

TEST(order_by, key_outside_the_select_list)
{
    auto db = make_db();

    tst::check_eq(db.rows("select name from common.t order by id"),
                  "n1 | n2 | n3 | n4 | n5 | nullid | ", "ordered by a column not selected");
}

TEST(order_by, multiple_keys_with_mixed_directions)
{
    tst::TestDb db("order_by_multi");
    db.exec("create table common.t(grp integer, id integer);");

    const int grps[] = {1, 2, 1, 2, 1};
    const int ids[]  = {3, 1, 5, 2, 4};
    for (int i = 0; i < 5; ++i)
        db.exec("insert into common.t (grp, id) values (" + std::to_string(grps[i]) + ", " +
                std::to_string(ids[i]) + ");");

    tst::check_eq(db.rows("select grp, id from common.t order by grp asc, id desc"),
                  "1 5 | 1 4 | 1 3 | 2 2 | 2 1 | ", "grp ascending, id descending");
    tst::check_eq(db.rows("select grp, id from common.t order by grp desc, id asc"),
                  "2 1 | 2 2 | 1 3 | 1 4 | 1 5 | ", "grp descending, id ascending");
}

TEST(order_by, ties_are_deterministic)
{
    auto db = make_db();

    tst::check_eq(db.rows("select id from common.t order by grp"),
                  "3 | 1 | 5 | 2 | 4 | NULL | ", "equal keys keep insertion order");
    tst::check_eq(db.rows("select id from common.t order by grp desc"),
                  "3 | 1 | 5 | 2 | 4 | NULL | ", "the tiebreaker ignores the key direction");
}

TEST(order_by, combined_with_where)
{
    auto db = make_db();

    tst::check_eq(db.rows("select id from common.t where id >= 2 order by id"),
                  "2 | 3 | 4 | 5 | ", "filter below the sort");
    tst::check_eq(db.rows("select id from common.t where id >= 2 order by id desc"),
                  "5 | 4 | 3 | 2 | ", "filter below a descending sort");
}

TEST(order_by, text_keys)
{
    auto db = make_db();

    tst::check_eq(db.rows("select name from common.t order by name"),
                  "n1 | n2 | n3 | n4 | n5 | nullid | ", "text ascending");
    tst::check_eq(db.rows("select name from common.t order by name desc"),
                  "nullid | n5 | n4 | n3 | n2 | n1 | ", "text descending");
}

TEST(order_by, unknown_column_is_rejected)
{
    auto db = make_db();

    const auto message = db.error("select id from common.t order by nosuchcol");
    tst::check(message.find("nosuchcol") != std::string::npos,
               "the error names the missing column, rather than reaching the executor");
}

TEST(order_by, virtual_tables)
{
    auto db = make_db();

    tst::check_eq(db.count("select table_name from information_schema.tables order by table_name"),
                  1, "information_schema can be ordered");
}
