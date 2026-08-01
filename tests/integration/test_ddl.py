#!/usr/bin/env python3
from common import *

reset("ddltest")
create_db("ddltest")

# CREATE TABLE with all supported types
check("create table all types", run("ddltest",
    "create table types_test("
    "  id integer primary key,"
    "  name text not null,"
    "  score real,"
    "  active bool,"
    "  code char"
    ");"))

# Duplicate table
check("duplicate table", run("ddltest", "create table types_test(id integer primary key);"), expect_ok=False)

# DROP TABLE
check("drop table", run("ddltest", "drop table types_test;"))
check("select dropped table", run("ddltest", "select * from types_test;"), expect_ok=False)

# CREATE INDEX / DROP INDEX
check("create table for index", run("ddltest", "create table idx_test(id integer primary key, val integer);"))
check("create index",           run("ddltest", "create index idx_val on idx_test(val);"))
check("duplicate index",        run("ddltest", "create index idx_val on idx_test(val);"), expect_ok=False)
check("drop index",             run("ddltest", "drop index idx_val on idx_test;"))
check("drop nonexistent index", run("ddltest", "drop index idx_val on idx_test;"), expect_ok=False)
check("drop table with index",  run("ddltest", "drop table idx_test;"))

# ALTER TABLE ADD COLUMN
check("create table for alter",  run("ddltest", "create table alter_test(id integer primary key);"))
check("alter add column",        run("ddltest", "alter table alter_test add column extra text;"))
check("insert after alter",      run("ddltest", "insert into alter_test (id, extra) values(1, 'hello');"))
out = run("ddltest", "select * from alter_test;")
check("select after alter",      out, expected_substr="hello")

summary()