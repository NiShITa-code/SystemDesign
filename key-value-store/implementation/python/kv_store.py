"""Educational distributed key-value store simulation.

Implements:
- consistent hashing placement
- replication with N replicas
- quorum reads/writes (R/W)
- simple vector-clock based version conflict detection

This is intentionally small and not production-ready.
"""

from __future__ import annotations

from dataclasses import dataclass
from hashlib import md5
from typing import Dict, List, Tuple

Clock = Dict[str, int]


def _hash(s: str) -> int:
    return int(md5(s.encode("utf-8")).hexdigest(), 16)


def _dominates(a: Clock, b: Clock) -> bool:
    """True when vector clock a is same/newer than b for all participants and newer for at least one."""
    keys = set(a) | set(b)
    ge_all = all(a.get(k, 0) >= b.get(k, 0) for k in keys)
    gt_any = any(a.get(k, 0) > b.get(k, 0) for k in keys)
    return ge_all and gt_any


@dataclass(frozen=True)
class VersionedValue:
    value: str
    clock: Tuple[Tuple[str, int], ...]

    @staticmethod
    def from_parts(value: str, clock: Clock) -> "VersionedValue":
        return VersionedValue(value=value, clock=tuple(sorted(clock.items())))

    def as_clock(self) -> Clock:
        return dict(self.clock)


class Node:
    def __init__(self, name: str) -> None:
        self.name = name
        self.store: Dict[str, List[VersionedValue]] = {}

    def put_version(self, key: str, vv: VersionedValue) -> None:
        existing = self.store.get(key, [])
        new_clock = vv.as_clock()

        kept: List[VersionedValue] = []
        for current in existing:
            current_clock = current.as_clock()
            if _dominates(new_clock, current_clock):
                # Newer write supersedes older version.
                continue
            if _dominates(current_clock, new_clock):
                # Existing version is newer; keep it.
                kept.append(current)
            else:
                # Sibling conflict version; keep both.
                kept.append(current)

        # Avoid duplicates.
        if vv not in kept:
            kept.append(vv)

        self.store[key] = kept

    def get_versions(self, key: str) -> List[VersionedValue]:
        return list(self.store.get(key, []))


class DistributedKVStore:
    def __init__(self, node_names: List[str], n_replicas: int = 3, w_quorum: int = 2, r_quorum: int = 2) -> None:
        if not node_names:
            raise ValueError("At least one node required")
        if n_replicas < 1:
            raise ValueError("n_replicas must be >= 1")

        self.nodes = [Node(name) for name in node_names]
        self.ring = sorted(((_hash(node.name), node) for node in self.nodes), key=lambda x: x[0])
        self.n = min(n_replicas, len(self.nodes))
        self.w = w_quorum
        self.r = r_quorum

    def _replicas_for(self, key: str) -> List[Node]:
        key_hash = _hash(key)
        start_idx = 0
        for i, (h, _) in enumerate(self.ring):
            if h >= key_hash:
                start_idx = i
                break

        replicas: List[Node] = []
        seen = set()
        i = start_idx
        while len(replicas) < self.n:
            _, node = self.ring[i % len(self.ring)]
            if node.name not in seen:
                replicas.append(node)
                seen.add(node.name)
            i += 1
        return replicas

    def put(self, key: str, value: str, coordinator: str, base_clock: Clock | None = None) -> Clock:
        replicas = self._replicas_for(key)
        clock = dict(base_clock or {})
        clock[coordinator] = clock.get(coordinator, 0) + 1
        version = VersionedValue.from_parts(value, clock)

        acks = 0
        for node in replicas:
            node.put_version(key, version)
            acks += 1

        if acks < self.w:
            raise RuntimeError(f"Write quorum not met: acks={acks}, W={self.w}")

        return clock

    def get(self, key: str) -> List[VersionedValue]:
        replicas = self._replicas_for(key)
        all_versions: List[VersionedValue] = []
        responses = 0
        for node in replicas:
            all_versions.extend(node.get_versions(key))
            responses += 1

        if responses < self.r:
            raise RuntimeError(f"Read quorum not met: responses={responses}, R={self.r}")

        # De-duplicate and retain only non-dominated versions.
        unique = list({v: None for v in all_versions}.keys())
        result: List[VersionedValue] = []
        for candidate in unique:
            cclock = candidate.as_clock()
            dominated = False
            for other in unique:
                if other == candidate:
                    continue
                if _dominates(other.as_clock(), cclock):
                    dominated = True
                    break
            if not dominated:
                result.append(candidate)
        return result
