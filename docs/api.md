# API

This document describes the public API of `vix::kv`.

`kv` is a durable, local-first key-value engine built for embedded and offline-first applications. It provides a simple developer-facing API on top of Softadastra Store.

---

## Namespace

The public API lives in:

```cpp
vix::kv
```

Main convenience header:

```cpp
#include <vix/kv/kv.hpp>
```

---

## Opening a database

### Default open

```cpp
auto db = vix::kv::open();
```

This opens a database with default options.

By default, `kv` uses a WAL-backed persistent store.

### Open with options

```cpp
vix::kv::api::KvOptions options;
options.path = "data/app.wal";

auto db = vix::kv::open(options);
```

---

## KvOptions

Header:

```cpp
#include <vix/kv/api/KvOptions.hpp>
```

`KvOptions` controls how the KV database is opened.

### Fields

#### `path`

```cpp
std::string path{"data/kv.wal"};
```

Path to the underlying WAL file.

Example:

```cpp
options.path = "./data/users.wal";
```

#### `enable_wal`

```cpp
bool enable_wal{true};
```

Enable or disable WAL persistence.

For normal usage, this should remain enabled.

#### `auto_flush`

```cpp
bool auto_flush{true};
```

Flush the WAL automatically after writes.

This improves durability at the cost of some write performance.

#### `initial_capacity`

```cpp
std::size_t initial_capacity{1024};
```

Hint for the initial in-memory index capacity.

Useful when you expect many keys.

---

## Kv

Header:

```cpp
#include <vix/kv/api/Kv.hpp>
```

`Kv` is the main handle used to interact with the database.

### Writing values

#### `set`

```cpp
bool set(const keys::KeyPath& key, const values::KvValue& value);
```

Insert or update a key.

Example:

```cpp
db.set({"users", "42"}, vix::kv::values::KvValue("Alice"));
```

This operation:

- encodes the structured key
- encodes the value
- appends the operation to the WAL
- updates the in-memory materialized state

Returns `true` on success.

### Reading values

#### `get`

```cpp
std::optional<values::KvValue> get(const keys::KeyPath& key) const;
```

Read a value by key.

Example:

```cpp
auto value = db.get({"users", "42"});

if (value)
{
  std::cout << value->to_string() << std::endl;
}
```

Returns `std::nullopt` if the key does not exist.

### Removing values

#### `erase`

```cpp
bool erase(const keys::KeyPath& key);
```

Delete a key.

Example:

```cpp
db.erase({"users", "42"});
```

Returns `true` on success.

### Listing by prefix

#### `list`

```cpp
std::vector<std::pair<keys::KeyPath, values::KvValue>>
list(const keys::KeyPath& prefix) const;
```

List all entries whose encoded key starts with the given prefix.

Example:

```cpp
auto users = db.list({"users"});

for (const auto& [key, value] : users)
{
  std::cout << value.to_string() << std::endl;
}
```

This is useful for collections such as:

- users
- products
- sessions
- orders

---

## KeyPath

Header:

```cpp
#include <vix/kv/keys/KeyPath.hpp>
```

`KeyPath` is the structured public representation of a key.

Example:

```cpp
vix::kv::keys::KeyPath key{"users", "42", "profile"};
```

Internally, it is flattened into a store key string.

### Supported operations

#### Construct empty

```cpp
KeyPath key;
```

#### Construct from segments

```cpp
KeyPath key{"users", "42"};
```

#### Append segment

```cpp
key.push_back("profile");
```

#### Access segments

```cpp
key.at(0);
key.front();
key.back();
key.segments();
```

#### Inspect

```cpp
key.empty();
key.size();
```

---

## KvValue

Header:

```cpp
#include <vix/kv/values/KvValue.hpp>
```

`KvValue` is the developer-facing value wrapper.

It is binary-safe and built on top of raw bytes.

### Construct from string

```cpp
vix::kv::values::KvValue value("Alice");
```

### Construct from bytes

```cpp
vix::kv::values::KvValue::Bytes bytes{1, 2, 3};
vix::kv::values::KvValue value(bytes);
```

### Helpers

#### `from_string`

```cpp
auto value = vix::kv::values::KvValue::from_string("Alice");
```

#### `from_bytes`

```cpp
auto value = vix::kv::values::KvValue::from_bytes(bytes);
```

#### `to_string`

```cpp
std::string s = value.to_string();
```

#### `bytes`

```cpp
const auto& raw = value.bytes();
```

#### `size`

```cpp
auto n = value.size();
```

#### `empty`

```cpp
if (value.empty()) { }
```

---

## Durability model

`kv` is WAL-backed.

A successful `set()` or `erase()` means the operation has been appended through the Softadastra Store/WAL pipeline before being reflected in materialized state.

On restart, the store is rebuilt by replaying the WAL.

This gives:

- crash recovery
- deterministic state reconstruction
- durable local writes

---

## Recovery behavior

Recovery happens automatically when opening the database:

```cpp
auto db = vix::kv::open(options);
```

Internally, this triggers WAL replay through the underlying store engine.

---

## Minimal example

```cpp
#include <iostream>
#include <vix/kv/kv.hpp>

int main()
{
  auto db = vix::kv::open();

  db.set({"users", "1"}, "Alice");
  db.set({"users", "2"}, "Bob");

  auto user = db.get({"users", "1"});
  if (user)
  {
    std::cout << user->to_string() << std::endl;
  }

  auto users = db.list({"users"});
  for (const auto& [key, value] : users)
  {
    std::cout << value.to_string() << std::endl;
  }

  db.erase({"users", "2"});

  return 0;
}
```

---

## Current API status

Available now:

- `open()`
- `open(options)`
- `set()`
- `get()`
- `erase()`
- `list()`
- `KeyPath`
- `KvValue`

Planned next:

- atomic operations
- compare-and-set
- watch
- sync bridge with Softadastra

---

## Design notes

`kv` is intentionally small.

It does not reimplement storage, recovery, or WAL logic. Instead, it builds a simple, ergonomic API on top of:

- `softadastra/store`
- `softadastra/wal`

This keeps the public interface clean while reusing a durable and deterministic core.

