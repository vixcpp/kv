# Softadastra

This document explains what **Softadastra** is in the context of `vix::kv`.

Softadastra is the durable systems foundation under `kv`.

It already provides the core building blocks that `kv` can reuse:

- `core`
- `fs`
- `wal`
- `store`
- `sync`
- `transport`

`kv` is not trying to rebuild these layers.
It is a simpler, developer-facing key-value API built on top of them.

---

## Why Softadastra exists

Most systems are built as if the environment is always stable:

- stable network
- ideal infrastructure
- central server always available

Softadastra is built for the opposite.

It is designed for:

- local-first execution
- durable writes
- deterministic recovery
- offline-capable systems
- future multi-node convergence

This is why it is the right foundation for `kv`.

---

## The Softadastra modules

## `core`

`core` provides the shared primitives used across the system.

Typical responsibilities include:

- timestamps
- base types
- shared utilities
- common low-level abstractions

This is the foundation layer everything else depends on.

---

## `fs`

`fs` handles filesystem observation and change modeling.

It makes it possible to:

- build snapshots of a directory
- compute diffs between snapshots
- represent file events like create, update, delete

This is what allows Softadastra to transform real filesystem changes into durable operations.

---

## `wal`

`wal` is the Write-Ahead Log layer.

This is one of the most important parts of Softadastra.

Its job is simple:

- append operations to a durable log
- preserve ordering
- make recovery possible after crash

The WAL is the source of truth for durable mutation history.

---

## `store`

`store` is the durable materialized state layer.

It sits above the WAL and gives the system:

- durable key/value-like entries
- replayable state
- versioned entries
- timestamped entries

`kv` directly benefits from this because it does not need to invent its own recovery or durability engine.

---

## `sync`

`sync` is the mutation coordination layer.

It is responsible for turning local operations into syncable operations that can be:

- queued
- batched
- acknowledged
- replayed remotely

This is the bridge between durable local state and multi-node convergence.

---

## `transport`

`transport` is the network delivery layer.

It is responsible for:

- binding nodes to ports
- connecting peers
- sending hello messages
- delivering sync batches
- polling incoming traffic

This means Softadastra is not just about local durability.
It is also preparing for real distributed communication.

---

## How the layers fit together

Softadastra can be understood like this:

```text
core
  ↓
fs → wal → store → sync → transport
```

Or, from the point of view of application data flow:

```text
filesystem change / local mutation
  ↓
WAL append
  ↓
materialized store state
  ↓
sync operation
  ↓
transport delivery
```

This layered design is what makes the system both durable and extensible.

---

## What the demos already show

The existing demos already prove that Softadastra is more than an idea.

They show that the modules can already work together in real flows:

### Basic store + sync + transport

A server can start:

- `StoreEngine`
- `SyncEngine`
- `TransportEngine`

and continuously poll for incoming sync traffic.

A client can:

- create a local operation
- submit it to sync
- generate a sync batch
- send that batch to the server

This demonstrates a real end-to-end path from local operation to replicated state. fileciteturn2file2 fileciteturn2file3

### Filesystem -> WAL -> Store -> Sync -> Transport

The more advanced drive demo goes even further.

On the client side, it:

- creates a real workspace on disk
- builds filesystem snapshots
- computes diffs
- serializes file events into the WAL
- updates the local store
- submits sync operations
- sends batches to the server

On the server side, it:

- receives replicated state through transport
- updates the store
- appends an audit WAL for observed mutations
- prints the converged final state

This shows a complete local-first pipeline from real files on disk to durable multi-node replication. fileciteturn2file0 fileciteturn2file1

### Direct sync bridge usage

There are also smaller examples where a local store operation is submitted to sync and then sent through transport to a peer, showing the intended bridge between `store`, `sync`, and `transport`. fileciteturn2file0

---

## What this means for `kv`

Because Softadastra already provides:

- durability
- replay
- versioned state
- batching
- peer transport
- end-to-end mutation flow

`kv` can stay focused on its own job:

- structured keys
- value encoding
- prefix listing
- simple embedded API
- ergonomic developer experience

That is exactly the right separation of responsibilities.

Softadastra handles the hard systems work.
`kv` makes it easy to use.

---

## Mental model

A good way to think about the relationship is:

- **Softadastra** = durable systems foundation
- **kv** = developer-facing local-first key-value layer

Or even shorter:

- Softadastra builds the engine
- `kv` builds the interface

---

## Why this matters

This architecture is strategically important.

It means `kv` is not limited to being a tiny embedded map.

Because it stands on Softadastra, it can grow toward:

- metadata-aware reads
- atomic operations
- watches
- sync-ready mutations
- multi-node convergence
- conflict handling

Without having to rewrite the system from scratch.

---

## Summary

Softadastra is the foundation under `vix::kv`.

It already provides real modules for:

- core primitives
- filesystem diffing
- WAL durability
- materialized store state
- sync batching
- peer-to-peer transport

`kv` is the simple API layer built on top of that foundation.

That is what makes `kv` small on the surface, but powerful underneath.

