//
// Parsing of ORDER BY / LIMIT / OFFSET, and the strictness of the select list.
//

#include "harness.hpp"

#include "lexer.hpp"
#include "parser.hpp"

using namespace types;

namespace
{
    std::string
    render(const SelectItem& item)
    {
        if (!item.is_aggregate())
            return item.column().value;

        const auto& call = item.aggregate();
        switch (call.kind)
        {
        case AggregateKind::COUNT_STAR: return "count(*)";
        case AggregateKind::COUNT:      return "count(" + call.column.value + ")";
        case AggregateKind::SUM:        return "sum(" + call.column.value + ")";
        case AggregateKind::AVG:        return "avg(" + call.column.value + ")";
        case AggregateKind::MIN:        return "min(" + call.column.value + ")";
        case AggregateKind::MAX:        return "max(" + call.column.value + ")";
        default:                        return "?";
        }
    }

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
            for (const auto& item : stmt.columns)
                out += render(item) + " ";
            out += "] group_by=[";
            for (const auto& col : stmt.group_by)
                out += col.value + " ";
            out += "] having=";
            out += stmt.having ? render(SelectItem{stmt.having->aggregate}) +
                                 "?" + stmt.having->literal.value
                               : "-";
            out += " order_by=[";
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
                  "cols=[] group_by=[] having=- order_by=[] limit=- offset=-", "bare star");
    tst::check_eq(parse_select("select a, b from t"),
                  "cols=[a b ] group_by=[] having=- order_by=[] limit=- offset=-", "column list");
    tst::check_eq(parse_select("select * from s.t where a == 1;"),
                  "cols=[] group_by=[] having=- order_by=[] limit=- offset=-", "schema, where, semicolon");
}

TEST(parser, order_by)
{
    tst::check_eq(parse_select("select * from t order by a"),
                  "cols=[] group_by=[] having=- order_by=[a:asc ] limit=- offset=-", "single key defaults to asc");
    tst::check_eq(parse_select("select * from t order by a desc"),
                  "cols=[] group_by=[] having=- order_by=[a:desc ] limit=- offset=-", "desc");
    tst::check_eq(parse_select("select * from t order by a asc, b desc"),
                  "cols=[] group_by=[] having=- order_by=[a:asc b:desc ] limit=- offset=-", "per-key direction");
    tst::check_eq(parse_select("select name from t where a == 1 order by age desc"),
                  "cols=[name ] group_by=[] having=- order_by=[age:desc ] limit=- offset=-", "after where");
    tst::check_eq(parse_select("select * from t order by a;"),
                  "cols=[] group_by=[] having=- order_by=[a:asc ] limit=- offset=-", "trailing semicolon");
}

TEST(parser, limit_and_offset)
{
    tst::check_eq(parse_select("select * from t limit 5"),
                  "cols=[] group_by=[] having=- order_by=[] limit=5 offset=-", "limit alone");
    tst::check_eq(parse_select("select * from t offset 3"),
                  "cols=[] group_by=[] having=- order_by=[] limit=- offset=3", "offset alone");
    tst::check_eq(parse_select("select * from t limit 5 offset 3"),
                  "cols=[] group_by=[] having=- order_by=[] limit=5 offset=3", "limit then offset");
    tst::check_eq(parse_select("select * from t limit 0"),
                  "cols=[] group_by=[] having=- order_by=[] limit=0 offset=-", "limit zero");
    tst::check_eq(parse_select("select * from t where a == 1 order by a desc limit 5 offset 3;"),
                  "cols=[] group_by=[] having=- order_by=[a:desc ] limit=5 offset=3", "every clause at once");
}

TEST(parser, aggregates)
{
    tst::check_eq(parse_select("select count(*) from t"),
                  "cols=[count(*) ] group_by=[] having=- order_by=[] limit=- offset=-",
                  "count star");
    tst::check_eq(parse_select("select count(id) from t"),
                  "cols=[count(id) ] group_by=[] having=- order_by=[] limit=- offset=-",
                  "count of a column");
    tst::check_eq(parse_select("select sum(x), avg(x), min(x), max(x) from t"),
                  "cols=[sum(x) avg(x) min(x) max(x) ] group_by=[] having=- order_by=[] limit=- offset=-",
                  "every aggregate at once");
    tst::check_eq(parse_select("select name, count(*) from t"),
                  "cols=[name count(*) ] group_by=[] having=- order_by=[] limit=- offset=-",
                  "a column and an aggregate side by side");
}

TEST(parser, group_by_and_having)
{
    tst::check_eq(parse_select("select name, count(*) from t group by name"),
                  "cols=[name count(*) ] group_by=[name ] having=- order_by=[] limit=- offset=-",
                  "one key");
    tst::check_eq(parse_select("select count(*) from t group by a, b"),
                  "cols=[count(*) ] group_by=[a b ] having=- order_by=[] limit=- offset=-",
                  "several keys");
    tst::check_eq(parse_select("select count(*) from t group by name having count(*) > 5"),
                  "cols=[count(*) ] group_by=[name ] having=count(*)?5 order_by=[] limit=- offset=-",
                  "having over count");
    tst::check_eq(parse_select("select sum(x) from t where x >= 1 group by name "
                               "having sum(x) > 10 limit 5 offset 2;"),
                  "cols=[sum(x) ] group_by=[name ] having=sum(x)?10 order_by=[] limit=5 offset=2",
                  "every clause at once");
}

TEST(parser, malformed_aggregates_are_rejected)
{
    tst::check(!accepted("select foo(x) from t"), "unknown function");
    tst::check(!accepted("select sum(*) from t"), "only count takes a star");
    tst::check(!accepted("select count( from t"), "unclosed call");
    tst::check(!accepted("select count(x from t"), "missing parenthesis");
    tst::check(!accepted("select count(*) from t group a"), "GROUP without BY");
    tst::check(!accepted("select count(*) from t group by"), "GROUP BY without a column");
    tst::check(!accepted("select count(*) from t group by a,"), "trailing comma");
    tst::check(!accepted("select count(*) from t having name > 5"), "HAVING without an aggregate");
    tst::check(!accepted("select count(*) from t having count(*)"), "HAVING without a comparison");
    tst::check(!accepted("select count(*) from t having count(*) > x"), "HAVING against a column");
}

// Aggregate names stay identifiers, so they are still usable as column names.
TEST(parser, aggregate_names_are_not_reserved)
{
    tst::check_eq(parse_select("select count from t"),
                  "cols=[count ] group_by=[] having=- order_by=[] limit=- offset=-",
                  "a column may be called count");
    tst::check(accepted("create table t(count integer, sum integer);"),
               "and declared with those names");
    tst::check(accepted("select min, max from t order by min"), "and ordered by");
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
