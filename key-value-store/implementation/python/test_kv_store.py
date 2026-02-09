import unittest

from kv_store import DistributedKVStore


class TestDistributedKVStore(unittest.TestCase):
    def test_put_get_single_value(self):
        kv = DistributedKVStore(["s1", "s2", "s3"], n_replicas=3, w_quorum=2, r_quorum=2)
        kv.put("user:1", "alice", coordinator="s1")
        versions = kv.get("user:1")
        self.assertEqual(1, len(versions))
        self.assertEqual("alice", versions[0].value)

    def test_concurrent_writes_create_conflict_versions(self):
        kv = DistributedKVStore(["s1", "s2", "s3"], n_replicas=3, w_quorum=2, r_quorum=2)

        base = kv.put("profile:name", "john", coordinator="s1")
        kv.put("profile:name", "johnSF", coordinator="s2", base_clock=base)
        kv.put("profile:name", "johnNY", coordinator="s3", base_clock=base)

        versions = kv.get("profile:name")
        values = sorted(v.value for v in versions)
        self.assertEqual(["johnNY", "johnSF"], values)

    def test_conflict_resolution_by_merge_write(self):
        kv = DistributedKVStore(["s1", "s2", "s3"], n_replicas=3, w_quorum=2, r_quorum=2)

        base = kv.put("name", "john", coordinator="s1")
        c1 = kv.put("name", "johnSF", coordinator="s2", base_clock=base)
        c2 = kv.put("name", "johnNY", coordinator="s3", base_clock=base)

        merged_clock = dict(c1)
        for node, version in c2.items():
            merged_clock[node] = max(merged_clock.get(node, 0), version)

        kv.put("name", "johnMerged", coordinator="s1", base_clock=merged_clock)

        versions = kv.get("name")
        self.assertEqual(1, len(versions))
        self.assertEqual("johnMerged", versions[0].value)


if __name__ == "__main__":
    unittest.main()
