#include "kv_store.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using kvdemo::Clock;
using kvdemo::DistributedKVStore;

void test_put_get_single_value() {
    DistributedKVStore kv({"s1", "s2", "s3"}, 3, 2, 2);
    kv.put("user:1", "alice", "s1");
    auto versions = kv.get("user:1");
    assert(versions.size() == 1);
    assert(versions[0].value == "alice");
}

void test_concurrent_conflict() {
    DistributedKVStore kv({"s1", "s2", "s3"}, 3, 2, 2);
    Clock base = kv.put("name", "john", "s1");
    kv.put("name", "johnSF", "s2", base);
    kv.put("name", "johnNY", "s3", base);

    auto versions = kv.get("name");
    std::vector<std::string> vals;
    for (const auto& v : versions) vals.push_back(v.value);
    std::sort(vals.begin(), vals.end());

    assert(vals.size() == 2);
    assert(vals[0] == "johnNY");
    assert(vals[1] == "johnSF");
}

void test_merge_resolution() {
    DistributedKVStore kv({"s1", "s2", "s3"}, 3, 2, 2);
    Clock base = kv.put("name", "john", "s1");
    Clock c1 = kv.put("name", "johnSF", "s2", base);
    Clock c2 = kv.put("name", "johnNY", "s3", base);

    Clock merged = c1;
    for (const auto& [k, v] : c2) {
        if (!merged.count(k) || merged[k] < v) merged[k] = v;
    }

    kv.put("name", "johnMerged", "s1", merged);
    auto versions = kv.get("name");
    assert(versions.size() == 1);
    assert(versions[0].value == "johnMerged");
}

int main() {
    test_put_get_single_value();
    test_concurrent_conflict();
    test_merge_resolution();
    std::cout << "All C++ KV tests passed\n";
    return 0;
}
