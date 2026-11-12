<<<<<<< HEAD
# Emergency Triage Queue

Comparative implementation project for an indexed binary min-heap, a lazy-invalidation priority queue, and an unsorted-array oracle.

## Layout

- `docs/interface.md` — authoritative interface contract for all queue implementations.
- `include/triage/priority_queue.hpp` — C++ interface matching the contract.
- `src/` — implementation location for the three queues and simulator.
- `tests/contract_example.cpp` — tiny shared behavioural test.

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Implementations A, B, and the oracle must satisfy the identical rules in `docs/interface.md` before comparative benchmarks are run.
=======
# Emergency_triage_queue

>>>>>>> 031c4f028287da5aacefcec3cb1e3211e81a56d3
