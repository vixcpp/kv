# Watch

This document explains the watch (reactive updates) model planned for `vix::kv`.

Today, `kv` provides a durable, local-first key-value API with:

- `set()`
- `get()`
- `erase()`
- `list()`

The next step is to allow applications to **react to changes** in real time.

---

## Why watch matters

Modern applications need to react to state changes:

- UI updates when data changes
- live dashboards
- cache invalidation
- session updates
- event-driven logic

Polling (`get()` in a loop) is inefficient and introduces latency.

`watch` provides a **push-based model**.

---

## Design goals

The watch system must follow the same core principles as `kv`:

- local-first
- deterministic behavior
- no hidden network dependency
- simple API
- built on durable mutations (WAL-backed)

---

## Scope

The initial watch model focuses on **local reactivity**.

It is not:

- distributed subscriptions
- cross-node streaming (yet)

It is:

- local observers of committed mutations

---

## Planned API shape

```cpp
auto handle = db.watch({"users"});

handle.on_change([](const ChangeEvent& ev) {
  std::cout << ev.key.to_string() << std::endl;
});
```

Also possible:

```cpp
auto handle = db.watch({"users", "42"});
```

---

## Core concepts

## Watch target

A watch can be attached to:

- a specific key
- a prefix

Examples:

```cpp
db.watch({"users"});        // all users
db.watch({"users", "42"});  // single user
```

---

## Change event

A watch delivers **change events** when mutations are committed.

A typical event may contain:

- key
- value (if applicable)
- operation type (`set` / `erase`)
- version (future)
- timestamp (future)

Example conceptual structure:

```cpp
struct ChangeEvent {
  KeyPath key;
  std::optional<KvValue> value;
  OperationType type;
};
```

---

## Trigger point

Events are emitted **only after a mutation is committed**.

This means:

- the change is already durable
- the state is already updated
- watchers never see partial state

---

## Deterministic delivery

Watch events must follow deterministic rules:

- events are delivered in commit order
- no duplication
- no missing events
- no reordering

This is critical for:

- correctness
- debugging
- future sync integration

---

## Local-only (first phase)

In the first version:

- watchers observe only local commits
- no remote events
- no cross-node propagation

Later, this can extend to sync.

---

## Prefix matching

If a watch is registered on a prefix:

```cpp
db.watch({"users"});
```

Then all matching keys trigger events:

- `{"users","1"}`
- `{"users","42","profile"}`

Prefix semantics must be strict and consistent with `list()`.

---

## Lifecycle

A watch handle should:

- be created via `watch()`
- remain active while in scope
- be cancellable

Example:

```cpp
auto handle = db.watch({"users"});

// later
handle.stop();
```

---

## Example use cases

## UI update

```cpp
db.watch({"users"}).on_change([](auto& ev) {
  refresh_ui();
});
```

---

## Cache invalidation

```cpp
db.watch({"products"}).on_change([](auto& ev) {
  cache.invalidate(ev.key);
});
```

---

## Logging

```cpp
db.watch({}).on_change([](auto& ev) {
  std::cout << "Change: " << ev.key.to_string() << std::endl;
});
```

---

## Internal flow (conceptual)

```
set / erase
  ↓
WAL append (durable)
  ↓
memory state update
  ↓
emit change event
  ↓
dispatch to watchers
```

---

## Threading model (future detail)

The implementation may:

- deliver events synchronously
- or queue them for async delivery

But must guarantee:

- ordering
- no data races in user callbacks (documented clearly)

---

## Relationship with WAL

Watch is tightly coupled to WAL semantics:

- WAL defines mutation order
- watch follows that order
- replay could also generate events (optional future behavior)

---

## Future extensions

Once sync is added, watch can evolve to:

- include remote mutations
- distinguish local vs remote changes
- support distributed subscriptions

---

## What watch is not

The first version of watch is not:

- a message broker
- a distributed event system
- a streaming platform

It is:

- a local reactive layer over durable state

---

## Mental model

Think of watch as:

- a subscription to the mutation log
- filtered by key or prefix
- delivered after commit

---

## Summary

Watch in `vix::kv` will provide:

- reactive updates
- prefix-based subscriptions
- deterministic event delivery
- durable-backed triggers
- simple API

This turns `kv` from a storage API into a **reactive state engine**.

