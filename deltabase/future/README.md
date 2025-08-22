# Future Features

This directory contains code for planned features that are not yet integrated into the main codebase.

## Threading (`threading/`)

Thread pool implementation for parallel query execution.

**Status**: Not integrated
**Reason**: Current focus is on core DBMS functionality. Multithreading will be added after the main features are stable.

### Files:
- `thread_pool.h` - Thread pool interface
- `thread_pool.c` - Thread pool implementation

### Integration Plan:
1. Complete core DBMS features (SQL parser, indexing, transactions)
2. Add comprehensive testing
3. Profile single-threaded performance
4. Integrate thread pool for parallel page processing
5. Benchmark and optimize

## Other Planned Features

- [ ] Advanced indexing (B+ trees)
- [ ] Query optimization
- [ ] Connection pooling
- [ ] Clustering support
- [ ] Backup/recovery system
