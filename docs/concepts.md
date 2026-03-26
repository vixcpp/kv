# Concepts

This document explains the core concepts behind `vix::kv`.

`kv` is not just a key-value store.
It is a **durable, local-first state engine** built on top of Softadastra.

Understanding these concepts is essential to use `kv` correctly.

---

## Local-first

`kv` is designed to work **without relying on a network**.

All operations happen locally:

- writes are executed locally
- reads are served locally
- no remote dependency

This means:

- your application works offline
- latency is minimal
- behavior is predictable

**Rule:** the network is optional, never required.

---

## Durability first

In `kv`, a write is not considered successful until it is durable.

Every mutation:

```
set / erase
  → encoded
  → appended to WAL
  → applied to memory
```

This guarantees:

- no data loss on crash
- safe recovery
- consistent state

**Rule:** persist first, then expose.

---

## Write-Ahead Log (WAL)

The WAL is the foundation of durability.

Instead of modifying state directly:

- every operation is appended to a log
- the log is the source of truth
- memory is a derived view

On restart:

- the WAL is replayed
- the state is rebuilt deterministically

**Key idea:** the log is the database.

---

## Deterministic state

`kv` guarantees that:

> the same sequence of operations always produces the same state

This is critical for:

- recovery
- debugging
- replication
- sync

No hidden randomness. No undefined behavior.

---

## Structured keys

Keys are not plain strings.

They are **paths of segments**:

```cpp
{"users", "42", "profile"}
```

This enables:

- hierarchical data modeling
- prefix queries
- logical grouping

Internally:

- keys are encoded into flat strings
- ordering is preserved

---

## Prefix-based queries

`kv` supports:

```cpp
db.list({"users"});
```

This returns all keys starting with `"users"`.

This replaces:

- table scans
- manual filtering

**Important:** prefix semantics must be exact and deterministic.

---

## Materialized state

`kv` keeps an in-memory representation of data.

This state:

- is built from the WAL
- is updated on every write
- is used for fast reads

So:

- WAL = source of truth
- memory = fast access layer

---

## Simple API over complex core

`kv` is intentionally minimal.

It does **not** implement:

- storage engine
- WAL logic
- recovery system

Instead, it reuses:

- `softadastra/store`
- `softadastra/wal`

This keeps:

- API simple
- behavior reliable
- implementation focused

---

## No hidden magic

Everything in `kv` follows explicit rules:

- writes are logged
- state is derived
- recovery is replay
- behavior is deterministic

There are:

- no background surprises
- no silent data changes
- no hidden network calls

---

## Toward sync

`kv` is designed to evolve.

Today:

- local durable state

Tomorrow:

- sync-ready mutations
- multi-node convergence
- conflict resolution

This is why:

- determinism matters
- WAL matters
- structured keys matter

---

## Mental model

Think of `kv` as:

- a log of operations
- a deterministic state machine
- a local database that never loses data

---

## Summary

`kv` is built on a few strong ideas:

- local-first execution
- durability before visibility
- WAL as source of truth
- deterministic state
- structured keys
- simple API

If you understand these, you understand `kv`.

