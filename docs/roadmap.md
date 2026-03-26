# Roadmap

This roadmap defines the development path of `kv`.

`kv` is not a standalone storage engine rebuilt from scratch.
It is the key-value layer of Vix built on top of Softadastra Store and WAL.

---

## Vision

`kv` starts small.

It evolves into:

- a durable local state engine
- a structured KV layer for Vix
- a sync-ready data model
- a bridge toward distributed local-first systems

---

## Principles

Every stage follows:

- local-first by default
- durable writes before acknowledgment
- deterministic behavior
- simple public API
- no duplicated storage logic
- reuse Softadastra modules

---

## v0.1.0

**Goal:** minimal, durable KV API

### Includes

- `open()`
- `open(options)`
- `set()`
- `get()`
- `erase()`
- `list(prefix)`
- `KeyPath`
- `KvValue`
- WAL-backed recovery
- tests + examples

### Result

Usable for:

- embedded storage
- sessions
- local cache
- offline persistence

---

## v0.2.0

**Goal:** correct key semantics

### Includes

- strict prefix matching
- path validation
- deterministic listing
- safer scans

### Result

Reliable hierarchical queries.

---

## v0.3.0

**Goal:** metadata-aware reads

### Includes

- version exposure
- timestamp exposure
- `get_entry()`
- `list_entries()`

### Result

Prepares for sync and concurrency control.

---

## v0.4.0

**Goal:** atomic operations

### Includes

- `atomic()` API
- compare-and-set
- multi-mutation commit
- version checks

### Example

```cpp
auto tx = db.atomic();
tx.check({"users","42"}, version);
tx.set({"users","42"}, "Alice");
auto ok = tx.commit();
```

### Result

Safe concurrent updates.

---

## v0.5.0

**Goal:** reactive watch

### Includes

- `watch(key)`
- `watch(prefix)`
- change events
- local observers

### Result

Real-time local reactivity.

---

## v0.6.0

**Goal:** TTL support

### Includes

- expiration metadata
- lazy expiration
- optional cleanup

### Result

Temporary state support (sessions, cache).

---

## v0.7.0

**Goal:** sync bridge

### Includes

- mutation export
- replay support
- sync mapping layer

### Result

Multi-node readiness.

---

## v0.8.0

**Goal:** conflict handling

### Includes

- merge policies
- last-writer-wins baseline
- custom resolvers

### Result

Deterministic distributed state.

---

## v0.9.0

**Goal:** developer experience

### Includes

- better docs
- debug tools
- logging hooks
- benchmarks

### Result

Production usability.

---

## v1.0.0

**Goal:** stable KV foundation

### Guarantees

- stable API
- stable durability model
- stable atomic ops
- stable watch
- stable sync baseline

### Result

Production-ready engine for:

- embedded apps
- offline-first systems
- local-first runtimes

---

## Development Order

Strict order:

1. core + setup
2. keys / values
3. engine integration
4. public API
5. metadata
6. atomic ops
7. watch
8. TTL
9. sync
10. conflict handling
11. stabilization

---

## Direction

`kv` evolves from:

small embedded KV

to:

durable local state engine

to:

sync-ready application layer

