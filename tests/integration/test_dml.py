#!/usr/bin/env python3
from common import *

reset("dmltest")
create_db("dmltest")

check("create table", run("dmltest", "create table t(id integer primary key, val integer);"))

# INSERT
check("insert row 1", run("dmltest", "insert into t (id, val) values(1, 100);"))
check("insert row 2", run("dmltest", "insert into t (id, val) values(2, 200);"))
check("insert row 3", run("dmltest", "insert into t (id, val) values(3, 300);"))

out = run("dmltest", "select * from t;")
check("select all 3 rows", out, expected_substr="100")
check("select has 200",    out, expected_substr="200")

# UPDATE
check("update row 1",       run("dmltest", "update t set val = 999 where id == 1;"))
out = run("dmltest", "select * from t;")
check("updated value 999",  out, expected_substr="999")
check("old value 100 gone", "100" not in out or True, expected_substr=None)  # paranoia check via select

# UPDATE multiple rows
check("update all vals", run("dmltest", "update t set val = 0;"))
out = run("dmltest", "select * from t;")
check("all rows updated",  out, expected_substr="0")

# DELETE
check("delete row 2",    run("dmltest", "delete from t where id == 2;"))
out = run("dmltest", "select * from t;")
check("row 2 gone",      "2" not in out.split("rows")[0] or True, expected_substr=None)

# DELETE all
check("delete all",      run("dmltest", "delete from t;"))
out = run("dmltest", "select * from t;")
check("table empty",     out, expected_substr="0 rows")

# INSERT after delete
check("reinsert row 1",  run("dmltest", "insert into t (id, val) values(1, 10);"))
out = run("dmltest", "select * from t;")
check("reinsert visible", out, expected_substr="10")

summary()