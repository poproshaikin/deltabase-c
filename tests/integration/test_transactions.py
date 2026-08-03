#!/usr/bin/env python3
from common import *

reset("txntest")
create_db("txntest")

check("create table", run("txntest", "create table t(id integer primary key, val integer);"))

# COMMIT: changes should persist
check("begin",         run("txntest", "begin;"))
check("insert in txn", run("txntest", "begin;", "insert into t (id, val) values(1, 10);", "commit;"))
out = run("txntest", "select * from t;")
check("committed row visible", out, expected_substr="10")

# ROLLBACK: changes should not persist
check("rollback txn", run("txntest", "begin;", "insert into t (id, val) values(2, 20);", "rollback;"))
out = run("txntest", "select * from t;")
check("rolled back row absent", out, expected_substr="1 row")
check("only 1 row after rollback", out, expected_substr="1 row")

# Rollback of UPDATE
check("update then rollback", run("txntest", "begin;", "update t set val = 999 where id == 1;", "rollback;"))
out = run("txntest", "select * from t;")
check("update rolled back", out, expected_substr="10")

# Rollback of DELETE
check("delete then rollback", run("txntest", "begin;", "delete from t where id == 1;", "rollback;"))
out = run("txntest", "select * from t;")
check("delete rolled back", out, expected_substr="10")

summary()