#!/usr/bin/env python3
from common import *

reset("fktest")
create_db("fktest")

check("create parent",        run("fktest", "create table parent(id integer primary key);"))
check("create child cascade", run("fktest", "create table child(id integer primary key, parent_id integer references parent(id) on delete cascade);"))
check("insert parent 1",      run("fktest", "insert into parent (id) values(1);"))
check("insert parent 2",      run("fktest", "insert into parent (id) values(2);"))
check("insert child 10->1",   run("fktest", "insert into child (id, parent_id) values(10, 1);"))
check("insert child 11->1",   run("fktest", "insert into child (id, parent_id) values(11, 1);"))
check("insert child 12->2",   run("fktest", "insert into child (id, parent_id) values(12, 2);"))

check("fk violation on insert",  run("fktest", "insert into child (id, parent_id) values(99, 999);"), expect_ok=False)
check("null fk allowed",         run("fktest", "insert into child (id, parent_id) values(100, null);"))

check("update parent id",    run("fktest", "update parent set id = 3 where id == 1;"))
out = run("fktest", "select * from parent;")
check("select parent has 3", out, expected_substr="3")

check("delete parent cascade",   run("fktest", "delete from parent where id == 2;"))
out = run("fktest", "select * from child;")
check("child row 12 removed",    out, expected_substr="10")

check("create child_r restrict", run("fktest", "create table child_r(id integer primary key, parent_id integer references parent(id) on delete restrict);"))
check("insert child_r 1->3",     run("fktest", "insert into child_r (id, parent_id) values(1, 3);"))
check("delete parent restricted", run("fktest", "delete from parent where id == 3;"), expect_ok=False)

summary()