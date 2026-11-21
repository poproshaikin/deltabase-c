//
// B+ tree index behaviour, including the page splits that no test used to reach.
//

#include "harness.hpp"

namespace
{
    tst::TestDb
    make_indexed_db(const std::string& prefix, int rows)
    {
        tst::TestDb db(prefix);
        db.exec("create table common.t(id integer primary key, name text);");

        for (int i = 0; i < rows; ++i)
            db.exec("insert into common.t (id, name) values (" + std::to_string(i) +
                    ", 'n" + std::to_string(i) + "');");

        return db;
    }
}

// The first split allocates a page while holding a reference to another one.
TEST(index, survives_a_leaf_split)
{
    auto db = make_indexed_db("index_leaf_split", 66);

    tst::check_eq(db.count("select * from common.t"), 66, "every row is readable");
    tst::check_eq(db.rows("select id from common.t where id == 42"), "42 | ", "index lookup");
    tst::check_eq(db.count("select id from common.t order by id"), 66, "sorted scan");
}

TEST(index, survives_internal_splits)
{
    auto db = make_indexed_db("index_internal_split", 3000);

    tst::check_eq(db.count("select * from common.t"), 3000, "every row is readable");
    tst::check_eq(db.rows("select id from common.t where id == 0"), "0 | ", "first key");
    tst::check_eq(db.rows("select id from common.t where id == 2999"), "2999 | ", "last key");
    tst::check_eq(db.rows("select id from common.t where id == 1500"), "1500 | ", "middle key");
    tst::check_eq(db.count("select id from common.t where id == 4000"), 0, "absent key");
    tst::check_eq(db.count("select id from common.t order by id"), 3000, "sorted scan");
    tst::check_eq(db.rows("select id from common.t order by id desc limit 3"),
                  "2999 | 2998 | 2997 | ", "bounded sort over an indexed table");
}

TEST(index, unique_constraint_is_enforced)
{
    auto db = make_indexed_db("index_unique", 66);

    const auto message = db.error("insert into common.t (id, name) values (42, 'dup');");
    tst::check(!message.empty(), "a duplicate primary key is rejected");
    tst::check_eq(db.count("select * from common.t"), 66, "and nothing was inserted");
}
