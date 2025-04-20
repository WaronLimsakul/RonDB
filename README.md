# RonDB

A minimalist, fast, and persistent SQL-like database engine written in C — featuring a custom B+ tree, pager, and low-level row storage, all built from scratch with zero dependencies.

---

## 💡 Why?

RonDB began as a deep dive into low-level database internals, inspired by [cstack/db_tutorial](https://github.com/cstack/db_tutorial). I used it as a foundation to explore how real databases work under the hood — then extended and polished the design to create a clean, educational, and fully persistent engine with my own twist.

Goals:
- Understand how storage engines work at the page level
- Implement a B+ tree with persistence
- Learn how SQL parsing maps to low-level operations

---

## ⚡ Quick Start

```bash
make run
```

You'll get interactive REPL:
```bash
> insert 1 alice alice@example.com
> select
id: 1 | name: alice | email: alice@example.com
> .btree
Tree:
- leaf (size 1)
  - 0 : 1
> .exit
exiting! Bye bye
```

---

## 🛠️ Usage

Supported commands:
  - `insert <id> <username> <email>`
  - `select`
  - `.btree` — debug view of the B+ tree
  - `.constants` — view internal constants

Technical specs:
  - Page size: 4096 bytes
  - B+ tree: handles insertions and splits
  - Data format: fixed-length row layout
  - Persistence: fully disk-backed

---

## 🤝 Contributing
This project was built for learning and showcasing low-level design — but PRs are welcome.
Possible extensions:
  - Add UPDATE, DELETE, or WHERE clauses
  - Implement transactions or concurrency
  - Add support for multiple tables
Feel free to fork and build on it. Open an issue if you want to discuss ideas

---
