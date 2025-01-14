# Building a sqlite with C

The Flow goes like this  
Cursor->table->pager->file
Each node is a page
And each cell(key-value pair) is a row accept for internal node which point to another node

## Table of Contents

- [Phase 1: REPL Interface]
- [Phase 2: SQL Compiler and Virtual Machine]
- [Phase 3: Single Table Database]
- [Phase 4: Persistent Database]
- [Phase 5: Cursor Abstraction]
- [Phase 6: B-tree Implementation]
- [Phase 7: Binary Search]
- [Phase 8: Node Splitting]
- [Phase 9: Recursively Searching the B-Tree]
- [Phase 10:Scanning a Multi-Level B-Tree Sequentially]
- [Phase 11: Updating Parent Node After a Split]
- [Phase 12: Splitting Internal Nodes]
- [Notes]

## Installation

To build and run the project, follow these steps:

1. Clone the repository:

   ```bash
   git clone <repository-url>
   cd <repository-directory>
   ```

2. Compile the code:

   ```bash
   gcc -o db database.c
   ```

3. Run the database:
   ```bash
   ./db <database-file>
   ```

## Usage

You can interact with the database using SQL commands. Here are some examples:

- To insert a new row:

  ```sql
  insert 1 "username" "email@example.com";
  ```

- To select rows:

  ```sql
  select;
  ```

- To exit the REPL:
  ```sql
  .exit
  ```

### A detailed explaination is provided in the logs.md

---
