# Key-Value Store – System Design

## 🧩 Problem
Design a distributed key-value store supporting:
- `put(key, value)`
- `get(key)`

Assumptions:
- Key-value pair size is small (< 10 KB).
- The system must support very large datasets with low latency.
- High availability and high scalability are required.
- Consistency should be tunable based on use case.

---

## ✅ Requirements

### Functional Requirements
- Store and retrieve values by unique key.
- Scale horizontally by adding/removing nodes.
- Replicate data for durability and availability.
- Support tunable consistency for read/write operations.

### Non-Functional Requirements
- Low read/write latency.
- High availability during node/network failures.
- Partition tolerance in distributed environments.
- Automatic rebalancing when topology changes.

---

## 🏗️ High-Level Design Choices

### 1) Data Partitioning
Use **consistent hashing** to distribute keys across a ring of nodes.

Why:
- Even key distribution.
- Minimal data movement during node join/leave.
- Supports heterogeneity via virtual nodes.

### 2) Replication
Replicate each key to **N** distinct nodes (clockwise on hash ring).

Why:
- Higher availability and durability.
- Better fault tolerance across nodes/data centers.

### 3) Consistency (Quorum)
Use quorum-based reads/writes:
- `N` = replication factor
- `W` = write acknowledgements required
- `R` = read acknowledgements required

Guidelines:
- If `W + R > N`, stronger consistency is achievable.
- If `W + R <= N`, latency improves but stale reads are more likely.

Recommended baseline:
- `N = 3`, `W = 2`, `R = 2` for balanced consistency/latency.

### 4) Consistency Model
Adopt **eventual consistency** by default for high availability.

Reason:
- Strong consistency can reduce availability under partition.
- Eventual consistency is common in Dynamo/Cassandra-style systems.

### 5) Conflict Resolution
Use **versioning** with **vector clocks** for concurrent write conflict detection.

- Detect ancestor vs sibling versions.
- Resolve conflicts at client/application layer or via merge policy.

---

## ⚖️ CAP Tradeoff Recommendation
In real systems, partition tolerance is mandatory.
So practical systems choose between:
- **CP** (consistency + partition tolerance): safer correctness, lower availability during partition.
- **AP** (availability + partition tolerance): always responsive, may serve stale data.

For a general-purpose key-value store in this repo, document both and choose per use case.
Default interview recommendation: **AP + eventual consistency + quorum tuning**.

---

## 🧱 Failure Handling

### Failure Detection
Use **gossip protocol** with heartbeats and membership lists.

### Temporary Failures
Use:
- **Sloppy quorum**: route to healthy nodes when ideal replicas are down.
- **Hinted handoff**: replay writes to recovered node later.

### Permanent Failures / Replica Repair
Use anti-entropy with **Merkle trees**:
- Compare hashes hierarchically.
- Sync only divergent key ranges.

### Data Center Outage
Replicate across multiple data centers/regions so one region outage does not stop service.

---

## ✍️ Storage Engine Direction (Practical)
A pragmatic write/read path inspired by LSM-based systems:

### Write Path
1. Append write to commit log.
2. Write to in-memory table (memtable).
3. Flush memtable to immutable SSTables on disk.

### Read Path
1. Check in-memory cache/memtable.
2. Use Bloom filters to locate likely SSTables.
3. Read from SSTables and reconcile latest version.

---

## 📦 What to include in this repo for this topic

### Must have
- This README with requirements, tradeoffs, and architecture decisions.
- One architecture diagram (hash ring, replication, coordinator, read/write flow).

### Nice to have
- Sequence diagrams for `put` and `get`.
- Failure scenario diagrams (partition, hinted handoff, repair).
- Small simulation (optional) for quorum behavior and conflict resolution.
- Included mini implementation under `key-value-store/implementation/`.

### Optional mini implementation
A minimal educational implementation can include:
- In-memory consistent hash ring.
- `put/get` with configurable `N/W/R`.
- Simple version metadata and conflict detection.
- Tests for quorum and conflict behavior.

Keep this intentionally small and clearly marked as non-production.

Implemented in this repo:
- `key-value-store/implementation/kv_store.py`
- `key-value-store/implementation/test_kv_store.py`
- `key-value-store/implementation/cpp/kv_store.hpp`
- `key-value-store/implementation/cpp/test_kv_store.cpp`

---

## 📚 References
- Amazon DynamoDB: https://aws.amazon.com/dynamodb/
- Memcached: https://memcached.org/
- Redis: https://redis.io/
- Dynamo paper: https://www.allthingsdistributed.com/files/amazon-dynamo-sosp2007.pdf
- Cassandra: https://cassandra.apache.org/
- Bigtable paper: https://static.googleusercontent.com/media/research.google.com/en//archive/bigtableosdi06.pdf
- Merkle tree: https://en.wikipedia.org/wiki/Merkle_tree
- SSTable intro: https://www.igvita.com/2012/02/06/sstable-and-log-structured-storage-leveldb/
- Bloom filter: https://en.wikipedia.org/wiki/Bloom_filter
