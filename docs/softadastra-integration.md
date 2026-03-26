# Softadastra Integration

This document explains how `vix::kv` integrates with Softadastra.

`kv` is not a standalone database.

It is a **thin layer built on top of Softadastra primitives**, reusing:

- Softadastra Store
- Softadastra WAL

This integration is the foundation of its durability and future sync capabilities.

---

## Overview

```
Kv API
  ↓
KvEngine
  ↓
Softadastra Store
  ↓
Softadastra WAL
```

`kv` delegates persistence and recovery entirely to Softadastra.

---

## Why Softadastra

Softadastra provides:

- durable local writes
- WAL-based persistence
- deterministic replay
- offline-first guarantees

Instead of rebuilding these complex systems, `kv` reuses them.

---

## Store integration

`kv` uses Softadastra Store as its storage backend.

Responsibilities of Store:

- append operations
- manage WAL
- replay on startup
- expose durable state

`kv` interacts with Store through a simple interface.

---

## WAL integration

Every mutation in `kv` becomes a WAL record.

Example:

```
set({"users","42"}, "Alice")
```

becomes:

```
WAL append → encoded mutation
```

This ensures:

- durability before visibility
- crash recovery
- ordered operations

---

## Recovery model

When opening the database:

```cpp
auto db = vix::kv::open();
```

Softadastra:

- loads WAL
- replays all operations
- rebuilds state

`kv` simply consumes the resulting state.

---

## Deterministic pipeline

Softadastra guarantees:

```
operation sequence → same final state
```

`kv` relies on this for:

- correctness
- reproducibility
- sync readiness

---

## Separation of responsibilities

### kv

- API
- key/value encoding
- in-memory state
- developer ergonomics

### Softadastra

- persistence
- WAL
- recovery
- ordering guarantees

This separation keeps `kv` simple and reliable.

---

## Write flow with Softadastra

```
kv.set()

→ encode key/value
→ call Store append
→ WAL write (Softadastra)
→ update memory (kv)
```

---

## Read flow

```
kv.get()

→ read from in-memory state
```

No direct dependency on disk during reads.

---

## Sync readiness

Because Softadastra is:

- WAL-based
- deterministic
- operation-driven

`kv` can later:

- export mutations
- replay remote operations
- converge across nodes

This is the foundation for:

- multi-device sync
- offline-first distributed systems

---

## Future integration

Planned extensions:

- mutation export for sync
- integration with Softadastra Sync
- conflict resolution layer
- distributed convergence

---

## Mental model

Think of the system as:

- Softadastra = durability + sync foundation
- kv = developer-facing state layer

---

## Summary

- `kv` does not store data directly
- Softadastra handles durability
- WAL is the source of truth
- kv builds a simple API on top
- this enables offline-first + sync-ready systems

This integration is what makes `kv` powerful.

