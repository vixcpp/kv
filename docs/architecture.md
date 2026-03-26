# Architecture

This document describes the internal architecture of `vix::kv`.

`kv` is a thin, deterministic layer built on top of Softadastra primitives.
It does not implement storage itself. Instead, it composes existing durable components into a simple API.

---

## Overview

```
Kv API
  ↓
Key / Value layer
  ↓
KvEngine
  ↓
Softadastra Store
  ↓
Write-Ahead Log (WAL)
  ↓
Disk
```

Each layer has a clear responsibility.

---

## Layers

### 1. Kv API

Public interface:

- `open()`
- `set()`
- `get()`
- `erase()`
- `list()`

Responsibilities:

- expose a simple developer API
- validate inputs
- orchestrate operations

This layer is intentionally minimal.

---

### 2. Key / Value layer

Components:

- `KeyPath`
- `KeyEncoder`
- `KvValue`
- `ValueCodec`

Responsibilities:

- transform structured keys into encoded keys
- transform values into binary-safe representation

Key idea:

Structured input → deterministic encoded format

---

### 3. KvEngine

Core internal engine:

- bridges API and store
- applies mutations
- maintains materialized state

Responsibilities:

- call encoding layer
- append operations to store
- update in-memory index
- serve reads

This is where `kv` logic lives.

---

### 4. Softadastra Store

External dependency:

- durable storage abstraction
- handles WAL integration
- manages replay

Responsibilities:

- persist operations
- replay WAL on startup
- provide append-only durability

---

### 5. WAL (Write-Ahead Log)

Core durability layer:

- append-only log
- sequential writes
- crash-safe

Responsibilities:

- store all mutations
- act as source of truth
- enable recovery

---

### 6. Materialized state (in-memory)

In-memory index:

- reconstructed from WAL
- updated on each write

Responsibilities:

- fast reads
- prefix scans
- deterministic state access

Important:

Memory is not the source of truth.
It is derived from the WAL.

---

## Write flow

```
db.set(key, value)

→ encode key
→ encode value
→ append to WAL (via store)
→ update memory index
→ return success
```

Guarantee:

A successful write is already durable.

---

## Read flow

```
db.get(key)

→ lookup in memory index
→ return value
```

Reads never touch disk.

---

## Delete flow

```
db.erase(key)

→ encode key
→ append delete operation to WAL
→ update memory index
```

Deletes are also durable operations.

---

## Prefix scan flow

```
db.list(prefix)

→ encode prefix
→ scan in-memory index
→ filter by prefix
→ return results
```

This enables hierarchical queries.

---

## Recovery flow

On startup:

```
open()

→ load WAL
→ replay operations
→ rebuild memory index
```

Guarantee:

State is reconstructed deterministically.

---

## Design choices

### Append-only log

Why:

- avoids corruption
- simplifies recovery
- guarantees ordering

---

### Deterministic encoding

Why:

- ensures consistent ordering
- enables prefix scans
- supports future sync

---

### Separation of concerns

`kv` does not:

- manage disk directly
- implement WAL logic

It delegates to Softadastra.

---

### Memory as cache

- fast reads
- rebuilt anytime
- never trusted as source of truth

---

## Future extensions

This architecture allows:

- atomic operations (version-based)
- watch system (event stream)
- sync bridge (mutation replication)
- conflict resolution (merge policies)

All without breaking the core model.

---

## Mental model

Think of `kv` as:

- a log of operations
- a deterministic state machine
- a materialized view over a WAL

---

## Summary

- WAL is the source of truth
- memory is derived state
- API is a thin layer
- Softadastra provides durability
- everything is deterministic

This architecture is what enables:

- offline-first behavior
- crash safety
- future sync capabilities

