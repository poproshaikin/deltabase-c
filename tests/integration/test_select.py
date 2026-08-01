#!/usr/bin/env python3
from common import *

reset("seltest")
create_db("seltest")

check("create table", run("seltest",
    "create table emp("
    "  id integer primary key,"
    "  name text not null,"
    "  dept integer,"
    "  salary real"
    ");"))

check("insert alice",   run("seltest", "insert into emp (id, name, dept, salary) values(1, 'alice', 10, 1000.0);"))
check("insert bob",     run("seltest", "insert into emp (id, name, dept, salary) values(2, 'bob',   20, 2000.0);"))
check("insert charlie", run("seltest", "insert into emp (id, name, dept, salary) values(3, 'charlie', 10, 1500.0);"))
check("insert dave",    run("seltest", "insert into emp (id, name, dept, salary) values(4, 'dave',  20, 3000.0);"))

# SELECT *
out = run("seltest", "select * from emp;")
check("select all has alice",   out, expected_substr="alice")
check("select all has dave",    out, expected_substr="dave")
check("select all 4 rows",      out, expected_substr="4 rows")

# SELECT specific columns
out = run("seltest", "select name, salary from emp;")
check("select cols has alice",  out, expected_substr="alice")
check("select cols has 2000",   out, expected_substr="2000")

# WHERE with equality
out = run("seltest", "select * from emp where id == 2;")
check("where id=2 has bob",     out, expected_substr="bob")
check("where id=2 one row",     out, expected_substr="1 rows")

# WHERE on non-pk column
out = run("seltest", "select * from emp where dept == 10;")
check("where dept=10 has alice",   out, expected_substr="alice")
check("where dept=10 has charlie", out, expected_substr="charlie")
check("where dept=10 two rows",    out, expected_substr="2 rows")

# WHERE is null
out = run("seltest", "select * from emp where salary is null;")
check("where is null empty", out, expected_substr="0 rows")

# SELECT from empty result
out = run("seltest", "select * from emp where id == 999;")
check("no match is empty", out, expected_substr="0 rows")

summary()