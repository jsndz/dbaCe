# Building a sqlite with C

The Flow goes like this  
Cursor->table->pager->file
Each node is a page
And each cell(key-value pair) is a row accept for internal node which point to another node

## Table of Contents

- [Phase 1: REPL Interface](#phase-1-repl-interface)
- [Phase 2: SQL Compiler and Virtual Machine](#phase-2-sql-compiler-and-virtual-machine)
- [Phase 3: Single Table Database](#phase-3-single-table-database)
- [Notes](#notes)

### A detailed explaination is provided in the logs.md

Here is the corrected README file based on the provided code and explanation:

---
