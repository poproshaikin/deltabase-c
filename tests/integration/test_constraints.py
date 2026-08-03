#!/usr/bin/env python3
from common import *

reset("contest")
create_db("contest")

# NOT NULL
check("create not null table", run("contest", "create table nn(id integer primary key, name text not null);"))
check("insert null into not null", run("contest", "insert into nn (id, name) values(1, null);"), expect_ok=False)
check("insert valid row",          run("contest", "insert into nn (id, name) values(1, 'alice');"))

# PRIMARY KEY = UNIQUE + NOT NULL
check("pk duplicate",     run("contest", "insert into nn (id, name) values(1, 'bob');"),  expect_ok=False)
check("pk not null",      run("contest", "insert into nn (id, name) values(null, 'bob');"), expect_ok=False)

# DEFAULT
check("create default table", run("contest", "create table def(id integer primary key, status integer default(0));"))
check("insert with default",  run("contest", "insert into def (id) values(1);"))
out = run("contest", "select * from def;")
check("default value present", out, expected_substr="0")

# UNIQUE constraint via separate UNIQUE keyword
check("create unique table", run("contest", "create table uq(id integer primary key, code integer unique);"))
check("insert unique 1",     run("contest", "insert into uq (id, code) values(1, 42);"))
check("insert unique 2",     run("contest", "insert into uq (id, code) values(2, 42);"), expect_ok=False)
check("insert unique null",  run("contest", "insert into uq (id, code) values(3, null);"))

# AUTOINCREMENT — all in one process (async flush not yet implemented)
out = run("contest",
    "create table ai(id integer primary key autoincrement, name text);",
    "insert into ai (name) values('first');",
    "insert into ai (name) values('second');",
    "select * from ai;")
check("autoincrement works", out, expected_substr="second")

summary()