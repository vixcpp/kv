# Why kv

This document explains **why `vix::kv` exists** and how it is different from traditional key-value stores.

---

## The problem

Most key-value systems are designed for:

- stable infrastructure
- centralized servers
- always-on networks
- ideal conditions

Examples include:

- Redis
- LevelDB
- RocksDB

These systems are powerful, but they assume a world that is always connected and reliable.

That is not the real world.

---

## The real world

In many environments:

- networks are unstable
- latency is unpredictable
- connections drop
- systems must work offline

Traditional systems struggle here because:

- they depend on servers
- they assume connectivity
- they treat durability and sync differently

---

## The idea behind kv

`kv` is built on a different model:

> local-first + durable + deterministic

Core idea:

- write locally
- persist immediately
- read instantly
- sync later

---

## What makes kv different

### 1. Local-first

Most systems:

- write → network → storage

`kv`:

- write → local → durable

No network required.

---

### 2. Durability by default

In `kv`:

- every write goes through WAL
- success means durable

No “eventual persistence”.

---

### 3. Deterministic behavior

Given the same operations:

- you always get the same result

This is critical for:

- recovery
- sync
- debugging

---

### 4. Built for offline

`kv` works:

- without internet
- without server
- without cluster

This is not an edge case.

This is the default.

---

### 5. Sync as an extension

Most systems:

- storage and sync are separate

`kv`:

- storage → WAL → sync

Same operation pipeline.

---

## kv vs Redis

Redis:

- in-memory first
- optional persistence
- server required

kv:

- durable first
- embedded
- no server

---

## kv vs LevelDB / RocksDB

LevelDB / RocksDB:

- storage engines
- no structured keys
- no built-in sync model

kv:

- structured keys
- WAL-first model
- designed for sync

---

## kv vs traditional databases

Traditional DBs:

- complex query engines
- heavy abstractions
- centralized mindset

kv:

- simple API
- predictable behavior
- local-first

---

## What kv is

kv is:

- a durable local state engine
- a structured key-value layer
- a foundation for offline-first apps
- a bridge to sync-ready systems

---

## What kv is not

kv is not:

- a full SQL database
- a distributed consensus system
- a cloud service
- a replacement for every database

---

## Why this matters

The world is moving toward:

- edge computing
- offline-first apps
- peer-to-peer systems
- unreliable networks

Systems must adapt.

---

## Vision

`kv` is part of a bigger direction:

- local-first computing
- durable state by default
- sync as a natural extension

---

## Mental model

Think of `kv` as:

- a durable log-backed map
- a deterministic state engine
- a building block for real-world systems

---

## Summary

`kv` exists because:

- the real world is not always online
- durability should not be optional
- systems must be predictable
- local-first is the future

`kv` is built for that world.

