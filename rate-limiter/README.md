# Rate Limiter – System Design

## 🧩 Problem

Design a rate limiter that allows a client to make at most **100 requests per minute** to an API.  
If the client exceeds this limit, the system should return **HTTP 429 – Too Many Requests**.

The purpose of a rate limiter is to:
- Prevent abuse
- Protect backend services from overload
- Ensure fair usage across users

---

## ✅ Requirements

### Functional Requirements
- Identify clients using user ID, IP address, or API key
- Enforce configurable rate limiting rules
- Reject excess requests with HTTP 429
- Return helpful rate limit headers (`X-RateLimit-*`)

### Non-Functional Requirements
- Very low latency (< 10ms per check)
- Highly available
- Scalable to ~50k requests/sec peak
- Eventual consistency is acceptable

---

## 🏗️ High-Level Architecture

![Rate Limiter Architecture](rate_limiter_architecture.png)

**Request Flow**

Client → API Gateway (Rate Limiter) → Redis → API Server → Client

The rate limiter is placed at the **API Gateway** so every request is checked before reaching backend services.

---

## ⚙️ Algorithm Choice

We evaluate common rate limiting algorithms:

| Algorithm | Drawback |
|---|---|
| Fixed Window Counter | Burst allowed at window boundaries |
| Sliding Window Log | High memory usage |
| Sliding Window Counter | Approximation complexity |
| **Token Bucket ✅** | Best balance of burst handling and memory efficiency |

We choose **Token Bucket** because it allows short bursts of traffic while maintaining a steady rate limit.

---

## 🗄️ Redis Data Model

Each client has a token bucket stored in Redis.

**Key**
user_id:bucket

**Fields**
- `tokens`
- `last_refill_timestamp`

Redis is used because it is:
- Extremely fast (in-memory)
- Shared across all gateway instances
- Supports TTL cleanup for inactive users
- Supports atomic operations

To prevent race conditions, the entire read–modify–write logic is implemented using a **Lua script** so the update is atomic.

---

## 📈 Scaling Strategy

A single Redis instance cannot handle very high traffic.

We scale using:
- **Redis Cluster / sharding**
- Consistent hashing on `user_id`

This ensures:
- Each client always maps to the same Redis shard
- Load is evenly distributed
- Horizontal scalability

---

## ❗ Failure Strategy (CAP Tradeoff)

If Redis is unavailable, we prefer **fail-closed**:
- Reject requests instead of risking backend overload

Redis replicas provide automatic failover for high availability.

---

## ⚡ Latency Optimizations

- Connection pooling between API Gateway and Redis
- Deploy Redis in the same region as gateways
- Avoid extra network calls during rate limit checks

---

## 🚧 Edge Cases Considered

- Hot keys from abusive users or shared NAT IPs
- Multiple rule types (per-user, per-IP, per-endpoint)
- Helpful HTTP 429 response headers
- Dynamic rule configuration support

---

## 🧠 Interview Concepts Demonstrated

This design demonstrates understanding of:

- Token Bucket algorithm
- Redis atomic operations using Lua scripting
- Distributed counters
- Consistent hashing and sharding
- CAP theorem tradeoffs
- API Gateway placement
- Horizontal scaling patterns


**Key**

