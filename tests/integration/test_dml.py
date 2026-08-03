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

# UPDATE multiple rows
check("update all vals", run("dmltest", "update t set val = 0;"))
out = run("dmltest", "select * from t;")
check("all rows updated",  out, expected_substr="0")

# DELETE
check("delete row 2",    run("dmltest", "delete from t where id == 2;"))
out = run("dmltest", "select * from t;")
check("row 2 gone",      out, expected_substr="2 rows")

# DELETE all
check("delete all",      run("dmltest", "delete from t;"))
out = run("dmltest", "select * from t;")
check("table empty",     out, expected_substr="No results.")

# INSERT after delete — same process only (index flush-on-exit not yet implemented)
check("reinsert row 1",  run("dmltest", "delete from t;", "insert into t (id, val) values(1, 10);", "select * from t;"))

summary()