# Key-Value Store Mini Implementation

This is a tiny educational prototype to complement the design document.

## What it demonstrates
- Consistent-hash ring based replica selection
- Replication factor `N`
- Quorum-style `W` and `R` checks
- Version conflict detection with vector clocks

## Run tests
```bash
cd key-value-store/implementation
python -m unittest -v
```

## Notes
- Not production-ready.
- No networking, persistence, compaction, or real failure injection.
- Built only to make design tradeoffs concrete.


## Available versions
- Python: `kv_store.py` + `test_kv_store.py`
- C++: `cpp/kv_store.hpp` + `cpp/test_kv_store.cpp`
