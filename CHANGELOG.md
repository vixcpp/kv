# Changelog

## v0.1.0

Initial release of **Vix KV**, a durable, local-first key-value engine built on top of Softadastra.

---

## Features

### KV API

- `set`, `get`, `erase`, `list`
- hierarchical keys via `KeyPath`
- prefix-based queries (`list(prefix)`)

### Values & Encoding

- `KvValue` (binary-safe abstraction)
- `KeyEncoder` (structured key encoding)
- `ValueCodec` (value serialization)

---

## Storage & Durability

- Built on top of **Softadastra Store**
- WAL-backed persistence (write-ahead log)
- Automatic recovery on startup
- Deterministic state reconstruction

---

## Engine

- `KvEngine`:
  - thin abstraction over `StoreEngine`
  - clear separation between API and storage
- Uses `apply_operation()` for mutation flow

---

## Developer Experience

Simple usage:

```cpp
auto db = vix::kv::open();
db.set({"users","1"}, vix::kv::values::KvValue("Alice"));
```

- automatic WAL directory creation
- minimal and predictable API
- no server required

---

## Examples

- `basic_kv.cpp` → basic usage
- `durable_restart.cpp` → persistence across restarts
- `prefix_scan.cpp` → prefix queries

---

## Build & Integration

- Full CMake support
- Compatible with Vix dependency system

Works with:

- monorepo usage
- Vix registry usage

---

## Internal Changes

- Removed unused components:
  - `KvMapper`
  - `KvOperation`
- Simplified architecture
- Aligned KV layer with Softadastra Store

---

## Documentation

Added:

- API documentation
- architecture overview
- concepts and use cases
- Softadastra integration

---

## Notes

This is the first stable version of **Vix KV**.

Focus of this release:

- correctness
- durability
- clean architecture

---

## Next

Planned improvements:

- transactions
- watch (reactive updates)
- sync integration (Softadastra)
- richer value handling

