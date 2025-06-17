# Documentation ***deltabase***

## Overview

DeltaBase is a custom database system implemented in C and C++. The system consists of several modules that communicate with each other and handle data management, storage, retrieval, and manipulation.

## System Architecture

### 1. Core

- Implemented in C.
- Fully responsible for interacting with the file system.
- Provides a low-level API used by the Executor.

### 2. C++ Modules

#### 1. Sql

- Responsible for parsing queries.
- **Lexical Analysis**: breaks the query into tokens, evaluates the type of each token (e.g., identifier, literal, etc.).
- **Syntactic Analysis**: builds an abstract syntax tree (AST) from the parsed tokens.

#### 2. Executor

- **Semantic Analysis**: checks for the existence of tables, columns, and type compatibility.
- **Query Evaluation**:
  - The executor receives the analyzed syntax tree and routes the queries to the core accordingly.
  - *Planned feature*: implement a query planner and optimizer to speed up execution, e.g., by removing redundant conditions like `1 == 1`.

### 3. Query Flow

- The client sends an SQL command to the server (via CLI or network) – *in progress*.
- The server passes the query to the Sql module for parsing.
- The Sql parser performs lexical and syntactic analysis, producing an AST.
- The Executor performs semantic analysis and validates the query.
- An optimized execution plan is prepared – *in progress*.
- The Executor calls the low-level API to access data.
- The Core reads/writes data from/to the file system and applies filters.
- The result is formatted and sent back to the client.

### 4. Future Extensions

- Query optimization (planner, optimizer)
- Transaction and rollback support
- TCP server
- Indexing
- Support for additional data types
