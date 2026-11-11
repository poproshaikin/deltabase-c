//
// Parsing of ORDER BY / LIMIT / OFFSET, and the strictness of the select list.
//

#include "harness.hpp"

#include "lexer.hpp"
#include "parser.hpp"

using namespace types;

namespace
{
    // Parses one statement and renders what came out, so a test can compare a literal.
    std::string
    parse_select(const std::string& query)
    {
        try
        {
            sql::SqlParser parser(sql::lex(query));
            AstNode node = parser.parse();

            if (node.type != AstNodeType::SELECT)
                return "not a select";

            const auto& stmt = std::get<SelectStmt>(node.value);

            std::string out = "cols=[";
            for (const auto& col : stmt.columns)
                out += col.value + " ";
            out += "] order_by=[";
            for (const auto& item : stmt.order_by)
                out += item.column.value + (item.desc ? ":desc " : ":asc ");
            out += "] limit=";
            out += stmt.limit ? std::to_string(*stmt.limit) : "-";
            out += " offset=";
            out += stmt.offset ? std::to_string(*stmt.offset) : "-";
            return out;
        }
        catch (const std::exception& ex)
        {
            return std::string("ERROR: ") + ex.what();
        }
    }

    bool
    accepted(const std::string& query)
    {
        try
        {
            sql::SqlParser parser(sql::lex(query));
            parser.parse();
            return true;
        }
        catch (const std::exception&)
        {
            return false;
        }
    }
}

TEST(parser, select_without_new_clauses)
{
    tst::check_eq(parse_select("select * from t"),
                  "cols=[] order_by=[] limit=- offset=-", "bare star");
    tst::check_eq(parse_select("select a, b from t"),
                  "cols=[a b ] order_by=[] limit=- offset=-", "column list");
    tst::check_eq(parse_select("select * from s.t where a == 1;"),
                  "cols=[] order_by=[] limit=- offset=-", "schema, where, semicolon");
}

TEST(parser, order_by)
{
    tst::check_eq(parse_select("select * from t order by a"),
                  "cols=[] order_by=[a:asc ] limit=- offset=-", "single key defaults to asc");
    tst::check_eq(parse_select("select * from t order by a desc"),
                  "cols=[] order_by=[a:desc ] limit=- offset=-", "desc");
    tst::check_eq(parse_select("select * from t order by a asc, b desc"),
                  "cols=[] order_by=[a:asc b:desc ] limit=- offset=-", "per-key direction");
    tst::check_eq(parse_select("select name from t where a == 1 order by age desc"),
                  "cols=[name ] order_by=[age:desc ] limit=- offset=-", "after where");
    tst::check_eq(parse_select("select * from t order by a;"),
                  "cols=[] order_by=[a:asc ] limit=- offset=-", "trailing semicolon");
}

TEST(parser, limit_and_offset)
{
    tst::check_eq(parse_select("select * from t limit 5"),
                  "cols=[] order_by=[] limit=5 offset=-", "limit alone");
    tst::check_eq(parse_select("select * from t offset 3"),
                  "cols=[] order_by=[] limit=- offset=3", "offset alone");
    tst::check_eq(parse_select("select * from t limit 5 offset 3"),
                  "cols=[] order_by=[] limit=5 offset=3", "limit then offset");
    tst::check_eq(parse_select("select * from t limit 0"),
                  "cols=[] order_by=[] limit=0 offset=-", "limit zero");
    tst::check_eq(parse_select("select * from t where a == 1 order by a desc limit 5 offset 3;"),
                  "cols=[] order_by=[a:desc ] limit=5 offset=3", "every clause at once");
}

TEST(parser, malformed_clauses_are_rejected)
{
    tst::check(!accepted("select * from t order a"), "ORDER without BY");
    tst::check(!accepted("select * from t order by"), "ORDER BY without a column");
    tst::check(!accepted("select * from t order by a,"), "trailing comma");
    tst::check(!accepted("select * from t order by 5"), "ordinal position is not supported");
    tst::check(!accepted("select * from t limit"), "LIMIT without a value");
    tst::check(!accepted("select * from t limit a"), "LIMIT with an identifier");
    tst::check(!accepted("select * from t limit 1.5"), "LIMIT with a real");
    tst::check(!accepted("select * from t limit 99999999999999999999999"), "LIMIT out of range");
    tst::check(!accepted("select * from t offset 3 limit 5"), "OFFSET before LIMIT");
}

// Reserved words used to break the select list silently: the loop stopped at the first
// non-identifier and the query was parsed as if those columns had not been written.
TEST(parser, select_list_is_strict)
{
    tst::check(!accepted("select order from t"), "a keyword alone must not become select *");
    tst::check(!accepted("select name, order from t"), "a keyword must not drop columns");
    tst::check(!accepted("select from t"), "no column list at all");
    tst::check(!accepted("select a, from t"), "dangling comma");
    tst::check(!accepted("select 5 from t"), "literal in the select list");
    tst::check(accepted("select * from t"), "star is still allowed");
    tst::check(accepted("select a, b from t"), "plain identifiers are still allowed");
}

// Quoting is the escape hatch for the words ORDER BY took over.
TEST(parser, quoted_identifiers)
{
    tst::check(accepted("create table t(\"order\" integer);"), "reserved word as a column name");
    tst::check(accepted("select \"order\" from t order by \"order\" desc"),
               "reserved word in the select list and in ORDER BY");
    tst::check(accepted("insert into t (\"order\") values (1)"), "reserved word in insert");
}

TEST(parser, other_statements_still_parse)
{
    tst::check(accepted("insert into t (a, b) values (1, 'x');"), "insert");
    tst::check(accepted("update t set a = 1 where b == 2;"), "update");
    tst::check(accepted("delete from t where a == 1;"), "delete");
    tst::check(accepted("create table t(a integer, b text);"), "create table");
    tst::check(accepted("alter table t add column c integer;"), "alter table");
    tst::check(accepted("drop table t;"), "drop table");
    tst::check(accepted("create index idx on t(a);"), "create index");
    tst::check(accepted("begin"), "begin");
    tst::check(accepted("commit"), "commit");
}
