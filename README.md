<table>
  <tr>
    <td valign="top" width="70%">

<h1>Vix kv</h1>

<p>
  <a href="https://x.com/vix_cpp">
    <img src="https://img.shields.io/badge/X-Follow-black?logo=x" />
  </a>
  <a href="https://www.youtube.com/@vixcpp">
    <img src="https://img.shields.io/badge/YouTube-Subscribe-red?logo=youtube" />
  </a>
  <img src="https://img.shields.io/badge/local--first-engine-green" />
  <img src="https://img.shields.io/badge/WAL-durable-blue" />
  <img src="https://img.shields.io/badge/C++-20-orange" />
</p>

<p>
<b>Vix kv</b> is a <b>durable, local-first key-value engine</b> built for real-world systems.
</p>

<p>
It provides <b>crash-safe writes</b>, <b>deterministic state</b>, and a <b>simple API</b>.
</p>

<p>
🌍 https://vixcpp.com<br/>
📘 <a href="./docs/getting-started.md">Getting Started</a> •
<a href="./docs/api.md">API</a> •
<a href="./docs/why-kv.md">Why kv</a>
</p>

</td>
<td valign="middle" width="30%" align="right">
<img src="https://res.cloudinary.com/dwjbed2xb/image/upload/v1762524350/vixcpp_etndhz.png" width="180"/>
</td>
</tr>
</table>

<hr />

## Built for the real world

Most databases assume stable networks and perfect infrastructure.

Reality is different.

`kv` is designed for:

- unreliable networks
- offline environments
- crash scenarios

Write locally. Persist first. Read instantly.

---

## Why developers use kv

- no data loss
- instant reads
- no server dependency
- works offline
- simple API

---

## Features

- Structured keys (`{"users","42"}`)
- Binary-safe values
- WAL-backed durability
- Prefix scan
- Crash recovery
- Embedded (no server required)

---

## Quick example

```cpp
#include <vix/kv/kv.hpp>

int main()
{
  auto db = vix::kv::open();

  db.set({"users","42"}, "Alice");

  auto value = db.get({"users","42"});

  if (value)
  {
    std::cout << value->to_string() << std::endl;
  }
}
```

---

## Documentation

- docs/getting-started.md
- docs/api.md
- docs/concepts.md
- docs/architecture.md
- docs/why-kv.md
- docs/use-cases.md
- docs/examples.md
- docs/transactions.md
- docs/watch.md
- docs/sync.md
- docs/internals.md
- docs/roadmap.md

---

## Installation

```bash
vix add @vix/kv
vix install
```

Inside a Vix project:

```bash
vix add @vix/kv
vix install
```

---

## Philosophy

- Local-first
- Durable by default
- Deterministic
- Simple

---

MIT License

