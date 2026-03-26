# Internals

This document describes the internal design of `vix::kv`.

It is intended for developers who want to understand how `kv` works under the hood.

---

## Overview

`kv` is a thin layer over Softadastra.

It is composed of:

- API layer
- encoding layer
- engine layer
- Softadastra backend

```
API → Encoding → Engine → Store → WAL
```

---

## Core components

### Kv

Public entry point.

Responsibilities:

- expose API
- forward operations to engine

---

### KvEngine

Central internal component.

Responsibilities:

- apply mutations
- coordinate encoding
- interact with store
- maintain in-memory index

---

### KeyPath

Represents structured keys.

Internally:

- vector of segments
- encoded into flat string

---

### KeyEncoder

Transforms:

```
KeyPath → encoded string key
```

Requirements:

- deterministic
- prefix-preserving ordering

---

### KvValue

Represents values.

Internally:

- byte buffer

Supports:

- string conversion
- raw bytes

---

### ValueCodec

Transforms:

```
KvValue → bytes → store format
```

---

## In-memory index

`kv` maintains a materialized view:

- map<encoded_key, value>
- updated on each mutation
- rebuilt from WAL on startup

---

## Mutation pipeline

### set()

```
input → encode key → encode value → append WAL → update memory
```

### erase()

```
input → encode key → append WAL(delete) → update memory
```

---

## WAL interaction

`kv` does not write files directly.

It calls Softadastra Store which:

- appends to WAL
- ensures durability
- provides replay

---

## Recovery

On open:

```
load WAL → replay → rebuild index
```

The engine processes each operation in order.

---

## Prefix scan

Listing works on encoded keys:

```
prefix → encode → scan map → match prefix
```

Requires encoding to preserve lexicographic order.

---

## Determinism

All operations must be:

- ordered
- reproducible
- side-effect free

This ensures:

- correct replay
- sync compatibility

---

## Memory model

- reads are served from memory
- writes update memory after WAL
- memory can be rebuilt anytime

---

## Threading (future)

Potential model:

- single-writer (simplest)
- multi-reader
- optional locks or queues

Must preserve:

- ordering
- consistency

---

## Error handling

Failures may occur during:

- WAL append
- encoding
- invalid input

Rules:

- no partial state updates
- return explicit failure

---

## Extension points

Internals allow future features:

- atomic transactions
- metadata tracking (version, timestamp)
- watch event emission
- sync integration

---

## Design constraints

- no duplication of Softadastra logic
- minimal surface API
- strong guarantees over features
- deterministic behavior always

---

## Mental model

Internally, `kv` is:

- a log-driven state machine
- a materialized map over WAL
- a deterministic engine

---

## Summary

Internals are simple by design:

- API is thin
- engine is small
- durability is delegated
- behavior is deterministic

This simplicity is what makes `kv` reliable and extensible.

