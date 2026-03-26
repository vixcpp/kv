# Sync

This document explains how synchronization is planned in `vix::kv`.

`kv` is designed as a **local-first state engine**.
Sync is an extension, not a requirement.

The system must work perfectly:

- without network
- without peers
- without sync

Then, sync is added on top.

---

## Philosophy

Sync in `kv` follows one core rule:

local state is the source of truth

This means:

- writes happen locally first
- durability happens locally
- sync happens later

Write locally. Persist first. Sync later.

---

## Why sync matters

Sync enables:

- multi-device state
- peer-to-peer systems
- offline-first collaboration
- distributed applications

But it must never break:

- determinism
- durability
- correctness

---

## Foundation: Softadastra

`kv` does not implement sync itself.

It relies on Softadastra:

- store → durable state
- wal → mutation history
- sync → mutation coordination
- transport → peer communication

---

## Sync model

Sync is based on **operations**, not state snapshots.

Each mutation:

```
set / erase
```

becomes:

```
operation → WAL → syncable unit
```

This allows:

- replay
- replication
- convergence

---

## Local mutation flow

```
db.set()

→ encode
→ WAL append
→ memory update
→ available for sync
```

---

## Sync flow

```
local mutation
  ↓
SyncEngine queue
  ↓
batch creation
  ↓
Transport send
  ↓
remote receive
  ↓
apply operation
```

---

## Operation-based replication

Instead of sending full state:

- only operations are sent
- ordering is preserved
- replay is deterministic

This is essential for:

- efficiency
- correctness
- conflict handling

---

## Deterministic convergence

Given the same operations:

all nodes must reach the same state

This requires:

- ordered operations
- deterministic application
- consistent encoding

---

## Acknowledgment model

Sync may use acknowledgments:

- operations are sent
- peers confirm receipt
- retries happen if needed

This ensures reliability over unstable networks.

---

## Batching

Operations are grouped:

```
next_batch()
```

This improves:

- network efficiency
- throughput
- stability

---

## Transport layer

Transport handles:

- peer discovery (future)
- connection
- message sending
- message receiving

Example:

```
connect_peer()
send_sync_batch()
poll_many()
```

---

## Offline-first behavior

If the network is unavailable:

- operations stay local
- WAL continues growing
- sync resumes later

No data is lost.

---

## Conflict handling (future)

When multiple nodes update the same key:

conflicts may happen

Future solutions include:

- last-writer-wins
- version-based resolution
- custom merge strategies

---

## Sync + kv relationship

`kv` provides:

- structured keys
- value encoding
- prefix queries
- simple API

Softadastra provides:

- sync engine
- WAL history
- transport

---

## Future evolution

Sync will evolve to support:

- multi-node convergence
- peer discovery
- conflict resolution
- partial replication
- selective sync

---

## Mental model

Think of sync as:

- replication of the WAL
- operation streaming between nodes
- deterministic replay everywhere

---

## Summary

Sync in `vix::kv`:

- is built on Softadastra
- is operation-based
- is deterministic
- is offline-first
- is optional but powerful

It transforms `kv` from a local store into a **distributed state system**.

