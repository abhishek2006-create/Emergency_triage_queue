# Emergency Triage Queue Common Contract

## Shared types

Both implementations include `src/common/triage_types.hpp` and use namespace `triage`.

- `PatientId`: `std::uint64_t`
- `PriorityKey`: `std::int64_t`
- `Timestamp`: `std::int64_t`
- `QueueEntry`: `{ patient_id, priority_key, arrival_time }`
- `QueueError`: `none`, `duplicate_id`, `missing_id`, `empty_queue`, `key_not_decreased`
- `ExtractResult`: `{ bool ok, QueueEntry entry, QueueError error }`

## Ordering

Ascending lexicographic order on `(priority_key, arrival_time, patient_id)`. The simulator calculates the effective key for its aging policy, not the queue.

## Queue methods

Both `triage::IndexedBinaryMinHeap` and `triage::LazyBinaryMinHeap` implement:

| Method | Return | Requirement |
| --- | --- | --- |
| `insert(const QueueEntry&)` | `QueueError` | Reject existing IDs |
| `extractMin()` | `ExtractResult` | Return and remove the smallest entry; report `empty_queue` if empty |
| `decreaseKey(PatientId, PriorityKey)` | `QueueError` | Reject missing IDs and keys not strictly smaller |
| `deletePatient(PatientId)` | `QueueError` | Remove an existing patient, or report `missing_id` |
| `buildHeap(const std::vector<QueueEntry>&)` | `QueueError` | Replace contents in linear expected time; reject duplicates without changing the old queue |
| `empty() const` | `bool` | Count live patients only |
| `size() const` | `std::size_t` | Number of live patients |
| `validate() const` | `bool` | Verify internal invariants |

Successful mutating calls return `QueueError::none`. Successful extractions return `ok=true`, `error=none`, and the extracted entry. Failed extractions return `ok=false` and `error=empty_queue`; the returned entry is not meaningful.

## Structure A

Located in `src/structA/`. Its `LocatorTable` is custom open addressing with linear probing and tombstones. Every valid entry satisfies `locator.find(heap[i].patient_id) == i`. All swaps update the locator.

## Structure B

Located in `src/structB/`. The heap is manually implemented; `std::unordered_map` is used only as the auxiliary version/state table. Updates insert a fresh version, and deletion pushes an invalid marker. Entries not matching the active version are ignored. Stale entries are discarded at the root and compaction occurs when stale entries outnumber live patients. `physicalSize() <= 2 * size()` after completed operations.

## Integration

No dependency on `include/triage/priority_queue.hpp` or `ITriageQueue`. Both queues expose identical callable methods and shared types. The Architect can use a template-based application runner or an explicit adapter for selection. The Oracle should use identical ordering, error semantics, and live-patient accounting.

The code is C++11 compatible. All tests in `tests/` must be compiled with the three implementation source files.

## Oracle

Located in `oracle/triage_oracle.hpp` and `oracle/triage_oracle.cpp`. `triage::TriageOracle` exposes the same public queue methods, uses an unsorted array (`std::vector<QueueEntry>`), and scans every live entry on `extractMin`. It uses independent lexicographic comparison logic. `buildHeap` validates all IDs before replacing the queue. The oracle's `validate()` checks uniqueness.

## Differential verification

`oracle/differential_test.cpp` generates a deterministic mixed-operation stream, compares Structure A, Structure B, and the Oracle after each operation, verifies error results, full returned entries, live sizes and invariants, and writes `oracle_failure_minimized.txt` if failure occurs. The default workload is 1,000,000 operations with seed `20261116`. The reducer removes subsets of the failing prefix while preserving the failure, using delta debugging.
