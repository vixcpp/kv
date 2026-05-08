# Changelog

All notable changes to Vix KV will be documented in this file.

The format is inspired by Keep a Changelog, and this project follows semantic versioning.

## v0.1.0

Initial public release of Vix KV.

### Added

- Added the core `vix::kv` public API.
- Added the main public include:

```cpp
#include <vix/kv/kv.hpp>
```

- Added simple durable API:

```cpp
auto kv = vix::kv::open("data/app");

kv.put("hello", "world");

auto value = kv.get("hello");
```

- Added explicit result-based API:

```cpp
auto opened = vix::kv::open_memory();

auto kv = opened.move_value();

auto result = kv.set({"hello"}, "world");
```

- Added public open helpers:
  - `vix::kv::open()`
  - `vix::kv::open(path)`
  - `vix::kv::open(options)`
  - `vix::kv::open_memory()`
  - `vix::kv::open_durable(path)`
  - `vix::kv::open_fast(path)`
- Added public API types:
  - `vix::kv::Kv`
  - `vix::kv::KvOptions`
  - `vix::kv::KvValue`
  - `vix::kv::KeyPath`
  - `vix::kv::KvResult<T>`
  - `vix::kv::KvError`
  - `vix::kv::KvErrorCode`
  - `vix::kv::KvStats`
- Added memory-only mode for tests and temporary state.
- Added durable mode with WAL-backed recovery.
- Added fast mode with manual flush control.
- Added structured keys through `KeyPath`.
- Added slash-key support in the simple API.
- Added string value support through `KvValue`.
- Added prefix listing.
- Added key deletion with tombstones.
- Added runtime statistics through `kv.stats()`.
- Added stable KV error codes.
- Added CRC32 checksum support.
- Added endian encoding helpers.
- Added key validation and key encoding.
- Added binary KV record encoding and decoding.
- Added in-memory table support.
- Added in-memory key index support.
- Added WAL writer.
- Added WAL reader.
- Added WAL recovery.
- Added data file writer and reader.
- Added immutable segment writer and reader.
- Added snapshot model, writer, and reader.
- Added manual compaction support.
- Added the internal `KvEngine`.
- Added public API tests.
- Added unit tests for records, keys, values, memtable, index, WAL, storage, snapshot, compaction, engine, and persistence.
- Added examples:
  - `basic.cpp`
  - `persistent.cpp`
  - `list.cpp`
  - `compact.cpp`

### Changed

- Simplified `KeyPath` usage for string literals.
- Simplified `MemTable` API to avoid ambiguous overloads.
- Simplified `KvIndex` API to avoid ambiguous overloads.
- Improved public `open(path)` convenience API.
- Improved `KvStats` to expose live keys, tombstones, WAL state, recovery counters, and operation counters.
- Updated examples to use the public `vix::kv/kv.hpp` API.

### Fixed

- Fixed missing example source references in `examples/CMakeLists.txt`.
- Fixed missing `std::span` include in snapshot reader.
- Fixed record decoder namespace/include issues.
- Fixed ambiguous initializer-list usage in examples and tests.
- Fixed public API tests to use `stats().wal_enabled` instead of `options().wal_enabled`.
- Fixed test layout to use a single clean CMake test entrypoint.

### Documentation

- Added API documentation draft for `docs.vixcpp.com/api/kv`.
- Added a focused README for the repository.
- Added examples showing:
  - simple put/get
  - structured keys
  - persistence after reopen
  - prefix listing
  - manual compaction

### Notes

This release focuses on correctness, a stable public API, and the local-first storage foundation.

The current scope is:

- local durable KV storage
- WAL recovery
- public C++ API
- structured keys
- snapshots
- segment storage
- manual compaction

Future releases will focus on tighter integration with the Vix runtime, stronger snapshot usage inside the engine, more advanced compaction policies, and richer diagnostics.
