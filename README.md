# Vix KV

Durable local-first key-value engine for Vix applications.

Vix KV provides a small public API for storing, reading, listing, and recovering local data safely.

```cpp
#include <vix/kv/kv.hpp>

int main()
{
  auto kv = vix::kv::open("data/app");

  kv.put("hello", "world");

  auto value = kv.get("hello");

  kv.close();
}
```

## Why Vix KV exists

Modern applications often need local state that survives restarts, network failures, crashes, and offline usage.

Vix KV is built for that use case:

- simple public API
- durable writes
- WAL-based recovery
- memory-only mode for tests
- structured keys
- deterministic behavior
- no heavy database dependency for basic local storage

It is designed as a small storage layer for Vix, Softadastra, and local-first systems.

## Features

- Durable key-value storage
- Write-ahead log recovery
- Memory-only mode
- Fast mode with manual flush
- Structured keys with KeyPath
- Prefix listing
- Tombstone-based deletes
- Segment storage
- Snapshot support
- Compaction support
- Public result-based API
- Simple direct API for application code

## Install

Add Vix KV to your project and link against:

```cmake
target_link_libraries(my_app PRIVATE vix::kv)
```

Include the public header:

```cpp
#include <vix/kv/kv.hpp>
```

## Quick start

### Simple durable API

```cpp
auto kv = vix::kv::open("data/app");

kv.put("users/1/name", "Ada");

auto name = kv.get("users/1/name");

if (name.has_value())
{
  std::cout << *name << '\n';
}

kv.close();
```

### Explicit result API

```cpp
auto opened = vix::kv::open_memory();

if (opened.is_err())
{
  std::cerr << opened.error().message() << '\n';
  return 1;
}

auto kv = opened.move_value();

auto written = kv.set({"users", "1", "name"}, "Ada");

if (written.is_err())
{
  std::cerr << written.error().message() << '\n';
  return 1;
}

auto value = kv.get({"users", "1", "name"});

if (value.is_ok())
{
  std::cout << value.value().to_string() << '\n';
}
```

## Open modes

### Durable

```cpp
auto kv = vix::kv::open("data/app");
```

Data is persisted and recovered after restart.

### Memory-only

```cpp
auto opened = vix::kv::open_memory();
```

Useful for tests and temporary state.

### Fast

```cpp
auto opened = vix::kv::open_fast("data/cache");
```

Durable storage with manual flush control.

```cpp
kv.set({"a"}, "one");
kv.set({"b"}, "two");

kv.flush();
```

## Public API

```cpp
vix::kv::open();
vix::kv::open(path);
vix::kv::open_memory();
vix::kv::open_durable(path);
vix::kv::open_fast(path);
```

```cpp
kv.put("key", "value");
kv.get("key");

kv.set({"key"}, "value");
kv.get({"key"});
kv.erase(vix::kv::KeyPath{"key"});
kv.contains({"key"});
kv.list({"prefix"});

kv.flush();
kv.close();

kv.size();
kv.empty();
kv.stats();
```

## Structured keys

```cpp
vix::kv::KeyPath key{"users", "1", "name"};

kv.set(key, "Ada");
```

Slash keys are also supported by the simple API:

```cpp
kv.put("users/1/name", "Ada");
```

## Persistence example

```cpp
const auto path = "data/app";

{
  auto kv = vix::kv::open(path);

  kv.put("hello", "world");
  kv.flush();
  kv.close();
}

{
  auto kv = vix::kv::open(path);

  auto value = kv.get("hello");

  if (value.has_value())
  {
    std::cout << *value << '\n';
  }

  kv.close();
}
```

Output:

```
world
```

## Build

```sh
vix build --build-target all -v
```

Or with CMake:

```sh
cmake -S . -B build
cmake --build build
```

## Tests

```sh
vix build --build-target all -v
ctest --test-dir build-ninja --output-on-failure
```

## Repository layout

```
include/vix/kv/
  api/          public API
  core/         result, errors, config, stats
  keys/         key path and encoding
  values/       value model and codec
  records/      record format
  wal/          write-ahead log
  storage/      data files and segments
  snapshot/     snapshot reader and writer
  compaction/   segment compaction
  internal/     KV engine

src/
  implementation files

tests/
  unit, wal, storage, snapshot, compaction, engine, api

examples/
  small usage examples
```

## Status

Vix KV is currently focused on correctness, recovery, and a stable public API.

Current scope:

- local durable storage
- WAL recovery
- public API tests
- segment and snapshot foundation
- compaction foundation

Future work may include stronger snapshot integration, advanced compaction policy, richer diagnostics, and tighter Vix runtime integration.

## License

MIT
