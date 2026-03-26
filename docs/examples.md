# Examples

This document provides practical examples for `vix::kv`.

The goal is to show how to use `kv` for common local-first and durable state workflows.

---

## 1. Basic set and get

```cpp
#include <iostream>
#include <vix/kv/kv.hpp>

int main()
{
  auto db = vix::kv::open();

  db.set({"users", "42"}, "Alice");

  auto value = db.get({"users", "42"});
  if (value)
  {
    std::cout << value->to_string() << std::endl;
  }

  return 0;
}
```

---

## 2. Store multiple users

```cpp
#include <iostream>
#include <vix/kv/kv.hpp>

int main()
{
  auto db = vix::kv::open();

  db.set({"users", "1"}, "Alice");
  db.set({"users", "2"}, "Bob");
  db.set({"users", "3"}, "Charlie");

  auto users = db.list({"users"});
  for (const auto& [key, value] : users)
  {
    std::cout << value.to_string() << std::endl;
  }

  return 0;
}
```

---

## 3. Delete a key

```cpp
#include <iostream>
#include <vix/kv/kv.hpp>

int main()
{
  auto db = vix::kv::open();

  db.set({"sessions", "abc123"}, "user:42");
  db.erase({"sessions", "abc123"});

  auto value = db.get({"sessions", "abc123"});
  if (!value)
  {
    std::cout << "session removed" << std::endl;
  }

  return 0;
}
```

---

## 4. Use structured keys

```cpp
#include <iostream>
#include <vix/kv/kv.hpp>

int main()
{
  auto db = vix::kv::open();

  db.set({"users", "42", "profile", "name"}, "Alice");
  db.set({"users", "42", "profile", "city"}, "Kampala");

  auto name = db.get({"users", "42", "profile", "name"});
  auto city = db.get({"users", "42", "profile", "city"});

  if (name && city)
  {
    std::cout << name->to_string() << " - " << city->to_string() << std::endl;
  }

  return 0;
}
```

---

## 5. Open with options

```cpp
#include <vix/kv/kv.hpp>

int main()
{
  vix::kv::api::KvOptions options;
  options.path = "data/app.wal";
  options.auto_flush = true;
  options.initial_capacity = 4096;

  auto db = vix::kv::open(options);

  db.set({"settings", "theme"}, "dark");

  return 0;
}
```

---

## 6. Store application settings

```cpp
#include <iostream>
#include <vix/kv/kv.hpp>

int main()
{
  auto db = vix::kv::open();

  db.set({"settings", "theme"}, "dark");
  db.set({"settings", "language"}, "en");
  db.set({"settings", "timezone"}, "Africa/Kampala");

  auto settings = db.list({"settings"});
  for (const auto& [key, value] : settings)
  {
    std::cout << value.to_string() << std::endl;
  }

  return 0;
}
```

---

## 7. Store session data

```cpp
#include <iostream>
#include <vix/kv/kv.hpp>

int main()
{
  auto db = vix::kv::open();

  db.set({"sessions", "token-123"}, "user:42");
  db.set({"sessions", "token-456"}, "user:51");

  auto session = db.get({"sessions", "token-123"});
  if (session)
  {
    std::cout << session->to_string() << std::endl;
  }

  return 0;
}
```

---

## 8. Prefix scan for grouped data

```cpp
#include <iostream>
#include <vix/kv/kv.hpp>

int main()
{
  auto db = vix::kv::open();

  db.set({"products", "1", "name"}, "Laptop");
  db.set({"products", "2", "name"}, "Phone");
  db.set({"products", "3", "name"}, "Tablet");

  auto products = db.list({"products"});
  for (const auto& [key, value] : products)
  {
    std::cout << value.to_string() << std::endl;
  }

  return 0;
}
```

---

## 9. Binary-safe values

```cpp
#include <iostream>
#include <vix/kv/kv.hpp>

int main()
{
  auto db = vix::kv::open();

  vix::kv::values::KvValue::Bytes raw{0x01, 0x02, 0x03, 0x04};
  db.set({"files", "blob-1"}, vix::kv::values::KvValue(raw));

  auto value = db.get({"files", "blob-1"});
  if (value)
  {
    std::cout << "size = " << value->size() << std::endl;
  }

  return 0;
}
```

---

## 10. Simple local cache

```cpp
#include <iostream>
#include <vix/kv/kv.hpp>

int main()
{
  auto db = vix::kv::open();

  db.set({"cache", "product:42"}, "cached-product-data");

  auto cached = db.get({"cache", "product:42"});
  if (cached)
  {
    std::cout << "cache hit: " << cached->to_string() << std::endl;
  }

  return 0;
}
```

---

## 11. User profile fields

```cpp
#include <iostream>
#include <vix/kv/kv.hpp>

int main()
{
  auto db = vix::kv::open();

  db.set({"users", "42", "name"}, "Alice");
  db.set({"users", "42", "city"}, "Kampala");
  db.set({"users", "42", "role"}, "admin");

  auto profile = db.list({"users", "42"});
  for (const auto& [key, value] : profile)
  {
    std::cout << value.to_string() << std::endl;
  }

  return 0;
}
```

---

## 12. Minimal durability workflow

```cpp
#include <iostream>
#include <vix/kv/kv.hpp>

int main()
{
  {
    auto db = vix::kv::open();
    db.set({"users", "1"}, "Alice");
  }

  {
    auto db = vix::kv::open();
    auto user = db.get({"users", "1"});
    if (user)
    {
      std::cout << "recovered: " << user->to_string() << std::endl;
    }
  }

  return 0;
}
```

---

## 13. Future transaction shape

```cpp
auto tx = db.atomic();
tx.check({"users", "42"}, expected_version);
tx.set({"users", "42"}, "Alice");
tx.erase({"locks", "current"});
auto ok = tx.commit();
```

---

## 14. Future watch shape

```cpp
auto handle = db.watch({"users"});

handle.on_change([](const ChangeEvent& ev) {
  std::cout << ev.key.to_string() << std::endl;
});
```

---

## 15. Future sync model

```cpp
// local write first
db.set({"notes", "1"}, "draft");

// sync comes later through Softadastra
```

---

## Summary

These examples show how `kv` can be used for:

- local state
- sessions
- cache
- grouped data
- durable persistence
- future reactive and sync-ready workflows

Start simple.
Persist first.
Grow toward sync later.

