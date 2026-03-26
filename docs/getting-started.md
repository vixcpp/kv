# Getting Started

This guide helps you get started with `vix::kv`.

`kv` is a durable, local-first key-value engine for Vix, built on top of Softadastra.

It is designed to be:

- simple to use
- durable by default
- ready for real-world conditions

---

## What you need

Before using `kv`, you need:

- a C++20 compiler
- a Vix project
- `vix` installed on your system

---

## Install kv

Add `kv` to your project:

```bash
vix add @vix/kv
vix install
```

---

## Include the main header

In your C++ file:

```cpp
#include <vix/kv/kv.hpp>
```

This gives you access to the public `kv` API.

---

## Open a database

The simplest way:

```cpp
auto db = vix::kv::open();
```

This opens a local database with default options.

By default, `kv` uses a WAL-backed durable store.

---

## Write your first value

```cpp
db.set({"users", "42"}, "Alice");
```

This stores the value `"Alice"` under the structured key `{"users", "42"}`.

---

## Read a value

```cpp
auto value = db.get({"users", "42"});

if (value)
{
  std::cout << value->to_string() << std::endl;
}
```

If the key exists, you get a `KvValue`.
If not, you get `std::nullopt`.

---

## Delete a value

```cpp
db.erase({"users", "42"});
```

This removes the key from the materialized state and records the delete durably.

---

## List by prefix

You can fetch all entries under a prefix:

```cpp
auto users = db.list({"users"});

for (const auto& [key, value] : users)
{
  std::cout << value.to_string() << std::endl;
}
```

This is useful for grouped data like:

- users
- sessions
- products
- settings

---

## Full example

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
    std::cout << "User 1: " << user->to_string() << std::endl;
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

## Open with options

You can configure the database with `KvOptions`:

```cpp
vix::kv::api::KvOptions options;
options.path = "data/app.wal";
options.auto_flush = true;

auto db = vix::kv::open(options);
```

Useful fields include:

- `path`
- `enable_wal`
- `auto_flush`
- `initial_capacity`

---

## What happens internally

When you call:

```cpp
db.set({"users", "42"}, "Alice");
```

`kv` does this:

1. encodes the structured key
2. encodes the value
3. appends the mutation to the WAL
4. updates the in-memory state

This means a successful write is already durable.

---

## Basic mental model

Think of `kv` like this:

- WAL = source of truth
- memory = fast read layer
- API = simple developer interface

Write locally. Persist first. Read instantly.

---

## Common patterns

### Store app settings

```cpp
db.set({"settings", "theme"}, "dark");
db.set({"settings", "language"}, "en");
```

### Store session data

```cpp
db.set({"sessions", "abc123"}, "user:42");
```

### Store user profile fields

```cpp
db.set({"users", "42", "name"}, "Alice");
db.set({"users", "42", "city"}, "Kampala");
```

---

## Current capabilities

Available now:

- `open()`
- `open(options)`
- `set()`
- `get()`
- `erase()`
- `list()`

Planned next:

- transactions
- watch
- sync
- conflict handling

---

## Next steps

After this guide, read:

- `docs/api.md`
- `docs/concepts.md`
- `docs/architecture.md`
- `docs/softadastra.md`

These documents explain how `kv` works and why it is built this way.

---

## Summary

With `kv`, you can start small:

- open a database
- write values
- read values
- list by prefix
- rely on durable local persistence

That is the foundation.

