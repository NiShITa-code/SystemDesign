#pragma once

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace kvdemo {

using Clock = std::map<std::string, int>;

inline bool dominates(const Clock& a, const Clock& b) {
    std::set<std::string> keys;
    for (const auto& [k, _] : a) keys.insert(k);
    for (const auto& [k, _] : b) keys.insert(k);

    bool gt_any = false;
    for (const auto& k : keys) {
        int av = a.count(k) ? a.at(k) : 0;
        int bv = b.count(k) ? b.at(k) : 0;
        if (av < bv) return false;
        if (av > bv) gt_any = true;
    }
    return gt_any;
}

struct VersionedValue {
    std::string value;
    Clock clock;

    bool operator==(const VersionedValue& other) const {
        return value == other.value && clock == other.clock;
    }
};

class Node {
   public:
    explicit Node(std::string name) : name_(std::move(name)) {}

    const std::string& name() const { return name_; }

    void putVersion(const std::string& key, const VersionedValue& vv) {
        auto& versions = store_[key];
        std::vector<VersionedValue> kept;

        for (const auto& current : versions) {
            if (dominates(vv.clock, current.clock)) {
                continue;
            }
            kept.push_back(current);
        }

        bool exists = false;
        for (const auto& v : kept) {
            if (v == vv) {
                exists = true;
                break;
            }
        }
        if (!exists) kept.push_back(vv);
        versions = std::move(kept);
    }

    std::vector<VersionedValue> getVersions(const std::string& key) const {
        auto it = store_.find(key);
        if (it == store_.end()) return {};
        return it->second;
    }

   private:
    std::string name_;
    std::unordered_map<std::string, std::vector<VersionedValue>> store_;
};

class DistributedKVStore {
   public:
    DistributedKVStore(const std::vector<std::string>& nodeNames, int nReplicas = 3, int wQuorum = 2,
                       int rQuorum = 2)
        : n_(nReplicas), w_(wQuorum), r_(rQuorum) {
        if (nodeNames.empty()) throw std::invalid_argument("At least one node required");
        if (nReplicas < 1) throw std::invalid_argument("nReplicas must be >= 1");

        for (const auto& n : nodeNames) {
            nodes_.emplace_back(n);
        }

        n_ = std::min<int>(n_, nodes_.size());

        for (size_t i = 0; i < nodes_.size(); ++i) {
            ring_.push_back({hash(nodes_[i].name()), static_cast<int>(i)});
        }
        std::sort(ring_.begin(), ring_.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    }

    Clock put(const std::string& key, const std::string& value, const std::string& coordinator,
              const Clock& baseClock = {}) {
        auto replicas = replicasFor(key);
        Clock clock = baseClock;
        clock[coordinator] = clock[coordinator] + 1;

        VersionedValue vv{value, clock};
        int acks = 0;
        for (auto* node : replicas) {
            node->putVersion(key, vv);
            ++acks;
        }

        if (acks < w_) throw std::runtime_error("Write quorum not met");
        return clock;
    }

    std::vector<VersionedValue> get(const std::string& key) const {
        auto replicas = replicasFor(key);
        int responses = 0;
        std::vector<VersionedValue> all;

        for (auto* node : replicas) {
            auto versions = node->getVersions(key);
            all.insert(all.end(), versions.begin(), versions.end());
            ++responses;
        }

        if (responses < r_) throw std::runtime_error("Read quorum not met");

        std::vector<VersionedValue> unique;
        for (const auto& v : all) {
            bool seen = false;
            for (const auto& u : unique) {
                if (u == v) {
                    seen = true;
                    break;
                }
            }
            if (!seen) unique.push_back(v);
        }

        std::vector<VersionedValue> result;
        for (const auto& c : unique) {
            bool dominated = false;
            for (const auto& o : unique) {
                if (&o == &c) continue;
                if (dominates(o.clock, c.clock)) {
                    dominated = true;
                    break;
                }
            }
            if (!dominated) result.push_back(c);
        }
        return result;
    }

   private:
    static std::size_t hash(const std::string& s) { return std::hash<std::string>{}(s); }

    std::vector<Node*> replicasFor(const std::string& key) const {
        std::size_t keyHash = hash(key);
        std::size_t start = 0;
        for (std::size_t i = 0; i < ring_.size(); ++i) {
            if (ring_[i].first >= keyHash) {
                start = i;
                break;
            }
        }

        std::vector<Node*> replicas;
        std::set<int> seen;
        std::size_t i = start;
        while (static_cast<int>(replicas.size()) < n_) {
            int idx = ring_[i % ring_.size()].second;
            if (!seen.count(idx)) {
                replicas.push_back(const_cast<Node*>(&nodes_[idx]));
                seen.insert(idx);
            }
            ++i;
        }
        return replicas;
    }

    std::vector<Node> nodes_;
    std::vector<std::pair<std::size_t, int>> ring_;
    int n_;
    int w_;
    int r_;
};

}  // namespace kvdemo
