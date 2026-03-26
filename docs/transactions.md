# Transactions

This document explains the transaction model planned for `vix::kv`.

Today, `kv` already provides durable single-key mutations:

- `set()`
- `erase()`

These operations are WAL-backed and deterministic.

The next step is to support **transactions**, so multiple related changes can be validated and committed as a single logical unit.

---

## Why transactions matter

Single operations are enough for many use cases.

But real applications often need more:

- update one key only if another key still has the expected version
- write multiple keys together
- delete one entry and create another in the same commit
- prevent lost updates in concurrent workflows

Without transactions, application code becomes fragile.

Transactions make these updates safer and easier to reason about.

---

## Design goals

The transaction model in `kv` should remain aligned with the overall philosophy of the project:

- local-first
- durable before acknowledgment
- deterministic behavior
- small public API
- explicit failure semantics
- built on Softadastra primitives, not a separate database engine

---

## Scope of transactions

The first transaction model for `kv` is expected to focus on **atomic mutation groups**.

That means:

- a transaction contains a list of checks and mutations
- the whole transaction either commits or fails
- no partial success is exposed
- durability still goes through the WAL/store pipeline

This is not about adding a full SQL-style transaction engine.

It is about adding safe application-level atomic state changes.

---

## Planned API shape

A likely developer-facing shape is:

```cpp
auto tx = db.atomic();

tx.check({"users", "42"}, expected_version);
tx.set({"users", "42"}, "Alice");
tx.erase({"locks", "current"});

auto ok = tx.commit();
```

This model is intentionally small:

- create transaction
- declare preconditions
- declare mutations
- commit once

---

## Core concepts

## Atomicity

Atomicity means:

> either the whole transaction is applied, or none of it is applied

If one check fails, the transaction must not partially update state.

This is the most important guarantee.

---

## Preconditions

A transaction should support checks before commit.

Examples:

- key exists
- key does not exist
- key version matches expected value
- key still has expected content

These checks are important for:

- compare-and-set
- optimistic concurrency
- lock-free coordination
- safe multi-step updates

---

## Deterministic commit

A transaction must produce deterministic results.

The same initial state and the same transaction input must always lead to the same outcome:

- success
- or failure

This matters for:

- debugging
- future sync
- replay
- correctness

---

## Durability

Transactions must still follow the same durability rule as the rest of `kv`:

> a successful commit must already be durable

That means commit must only report success after the transaction has gone through the durable append path.

Persist first. Then expose.

---

## No partial visibility

If a transaction modifies several keys, readers must not observe an intermediate state where only some updates are visible.

The visible result must be:

- old state, before commit
- or new state, after commit

Never an in-between state.

---

## Relationship with metadata

Transactions depend on metadata-aware state.

This is why metadata support comes before transactions in the roadmap.

Useful metadata includes:

- entry version
- entry timestamp

Version tracking is especially important for:

- compare-and-set
- optimistic concurrency checks
- deterministic conflict detection

---

## Example use cases

## Safe profile update

```cpp
auto tx = db.atomic();
tx.check({"users", "42"}, expected_version);
tx.set({"users", "42", "profile"}, "Alice");
tx.set({"users", "42", "updated_at"}, "1712345678");
auto ok = tx.commit();
```

This avoids overwriting newer state accidentally.

---

## Move ownership

```cpp
auto tx = db.atomic();
tx.erase({"locks", "resource-a"});
tx.set({"locks", "resource-b"}, "worker-1");
auto ok = tx.commit();
```

This ensures the lock move is not partially visible.

---

## Session renewal

```cpp
auto tx = db.atomic();
tx.check({"sessions", "abc"}, expected_version);
tx.set({"sessions", "abc"}, "new-payload");
tx.set({"sessions_meta", "abc", "expires_at"}, "1712349999");
auto ok = tx.commit();
```

This keeps related state consistent.

---

## Expected internal flow

A future transaction commit will likely follow a path like this:

```text
transaction definition
  ↓
validate preconditions against materialized state
  ↓
build deterministic mutation set
  ↓
append durable commit record / records
  ↓
apply new state atomically in memory
  ↓
return commit result
```

The exact implementation can evolve, but the guarantees must stay the same.

---

## Commit result

A transaction should probably return more than a raw boolean.

A richer result type may expose:

- success / failure
- reason for failure
- failed check index
- committed version information

Possible example:

```cpp
auto result = tx.commit();

if (!result.ok())
{
  std::cout << result.reason() << std::endl;
}
```

This helps application code react clearly.

---

## Failure semantics

Failure should be explicit and deterministic.

Examples of failure causes:

- expected version mismatch
- missing key
- duplicate creation attempt
- invalid mutation set
- internal durability failure

The system should never silently degrade into partial commit behavior.

---

## What transactions are not

At least in the first versions, transactions are **not** intended to become:

- a general query engine
- a relational system
- distributed consensus
- arbitrary rollback scripting
- unbounded isolation model complexity

The goal is smaller:

- safe local atomic mutation groups
- clear concurrency checks
- deterministic results

---

## Why this matters for the future

Transactions are a major step for `kv`.

They transform it from:

- simple durable key-value API

into something closer to:

- safe application state engine

They also prepare the ground for future features such as:

- watches
- sync bridge
- conflict handling
- distributed convergence

Because once state changes are grouped and explicit, they become easier to observe, replay, and replicate.

---

## Mental model

Think of a transaction in `kv` as:

- a guarded mutation batch
- validated against current local state
- committed durably
- applied atomically

---

## Summary

Transactions in `vix::kv` are planned to provide:

- atomic multi-key updates
- version-based checks
- deterministic commit behavior
- durable commits
- no partial visibility

This keeps the API small while making the system much safer for real applications.

