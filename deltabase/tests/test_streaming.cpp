//
// Which plans stream their result, and that the rows are the same either way.
//

#include "harness.hpp"

#include "execution_result.hpp"

namespace
{
    bool
    streams(engine::Engine& engine, const std::string& query)
    {
        auto result = engine.execute_query(query);
        return dynamic_cast<types::StreamedResult*>(result.get()) != nullptr;
    }
}

// The decision used to be read off the plan root, which after the projection step is never
// a scan, so every SELECT was materialised and the streaming path never ran at all.
TEST(streaming, large_seq_scans_stream)
{
    tst::TestDb db("streaming");
    db.exec("create table common.small(id integer);");
    db.exec("create table common.big(id integer);");

    for (int i = 0; i < 20; ++i)
        db.exec("insert into common.small (id) values (" + std::to_string(i) + ");");
    for (int i = 0; i < 3000; ++i)
        db.exec("insert into common.big (id) values (" + std::to_string(i) + ");");

    tst::check(!streams(db.engine(), "select * from common.small"),
               "a small result is materialised");
    tst::check(streams(db.engine(), "select * from common.big"),
               "a large seq scan streams");
    tst::check(streams(db.engine(), "select id from common.big"),
               "a projection does not stop it streaming");
    tst::check(!streams(db.engine(), "select * from information_schema.tables"),
               "virtual tables are materialised");

    tst::check_eq(db.count("select * from common.big"), 3000, "every row arrives when streamed");
    tst::check_eq(db.count("select * from common.small"), 20, "and when materialised");
}

// A streamed result owns its executor, so it has to open and close it itself. Before it did,
// a streamed scan returned nothing at all: the scan cursor was never initialised.
TEST(streaming, a_streamed_result_opens_its_executor)
{
    tst::TestDb db("streaming_rows");
    db.exec("create table common.t(id integer, name text);");
    for (int i = 0; i < 3000; ++i)
        db.exec("insert into common.t (id, name) values (" + std::to_string(i) + ", 'x');");

    tst::check(streams(db.engine(), "select * from common.t"), "the plan streams");
    tst::check_eq(db.count("select * from common.t"), 3000, "and yields all of its rows");
    tst::check_eq(db.count("select id from common.t order by id"), 3000,
                  "a sort above a streamed scan still returns everything");
    tst::check_eq(db.rows("select id from common.t order by id desc limit 3"),
                  "2999 | 2998 | 2997 | ", "and ordering is correct");
}

// Abandoning a result without reading it must not leave the database lock held, or the
// next statement in the session would deadlock.
TEST(streaming, an_abandoned_stream_releases_its_lock)
{
    tst::TestDb db("streaming_abandon");
    db.exec("create table common.t(id integer);");
    for (int i = 0; i < 3000; ++i)
        db.exec("insert into common.t (id) values (" + std::to_string(i) + ");");

    {
        auto result = db.engine().execute_query("select * from common.t");
        types::DataRow row;
        result->next(row);   // one row, then drop it on the floor
    }

    tst::check_eq(db.count("select * from common.t"), 3000,
                  "a later query still runs after an unread stream");
}
