# Use Cases

This document shows practical use cases for `vix::kv`.

`kv` is designed for real-world environments where:

- network is unreliable
- latency matters
- durability is required
- systems must work offline

---

## 1. Local application state

Use `kv` to store local app data safely.

Examples:

- user preferences
- app configuration
- local cache

```cpp
db.set({"settings","theme"}, "dark");
```

Why `kv`:

- no server required
- instant reads
- crash-safe

---

## 2. Session storage

Store sessions locally in embedded apps or services.

```cpp
db.set({"sessions","abc123"}, "user-data");
```

With future TTL support:

- automatic expiration
- safe updates

---

## 3. Offline-first apps

Apps that must work without internet:

- mobile apps
- desktop tools
- field systems

Workflow:

- write locally
- persist
- sync later

---

## 4. Edge computing

Run logic close to the user:

- IoT devices
- edge nodes
- local services

`kv` provides:

- durable storage
- predictable performance
- no external dependency

---

## 5. Local cache (durable)

Unlike in-memory caches:

- data survives restart
- no recomputation needed

```cpp
db.set({"cache","product:42"}, "data");
```

---

## 6. Event sourcing (lightweight)

Use WAL-backed operations as event log:

- record mutations
- replay state
- debug easily

---

## 7. CLI tools / dev tools

Tools that need persistent state:

- CLI apps
- build tools
- local automation

---

## 8. Embedded databases

Replace heavy databases in:

- small services
- single-node apps
- local systems

---

## 9. Real-time apps (with watch)

Future watch support enables:

- UI updates
- reactive systems
- live dashboards

---

## 10. Multi-device sync (future)

With Softadastra:

- sync state across nodes
- offline collaboration
- peer-to-peer apps

---

## 11. Locking / coordination

With transactions (future):

- distributed locks
- safe updates
- concurrency control

---

## 12. Data pipeline staging

Use `kv` as:

- staging area
- durable buffer
- intermediate store

---

## 13. Local-first SaaS backend

Instead of:

- central DB first

You can:

- store locally
- sync later

---

## 14. File sync systems

Combined with Softadastra FS:

- track file changes
- persist mutations
- sync across machines

---

## 15. Gaming / local state

- player state
- save files
- offline progress

---

## Why kv fits these use cases

Because it is:

- local-first
- durable by default
- deterministic
- simple
- embedded

---

## Summary

`kv` is useful anywhere you need:

- reliable local storage
- offline capability
- simple API
- future sync potential

It is not just a database.

It is a foundation for real-world systems.

