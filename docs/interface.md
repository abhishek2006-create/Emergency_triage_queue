# Queue Interface Contract

This document is the single behavioural contract for implementation **A** (indexed binary min-heap), implementation **B** (lazy-invalidation binary heap), and implementation **O** (unsorted-array oracle). All three implementations must return the same results for the same sequence of queue calls.

## 1. Common language, types, and entry format

All production code is C++20. The shared names and types are defined in `include/triage/priority_queue.hpp`.

| Field | Type | Meaning |
| --- | --- | --- |
| `patient_id` | `uint64_t` (`PatientId`) | Unique identifier of a patient currently waiting. Must be greater than zero. |
| `priority_key` | `int64_t` (`PriorityKey`) | Queue ordering key. **Smaller key wins.** The simulator calculates it; queues do not calculate aging or severity. |
| `arrival_time` | `int64_t` (`Timestamp`) | Simulated minutes from the simulation epoch. Used only for deterministic ties and reporting. |

```cpp
struct QueueEntry {
  PatientId patient_id;
  PriorityKey priority_key;
  Timestamp arrival_time;
};
```

No queue implementation may mutate `patient_id` or `arrival_time`. A `decreaseKey` changes only `priority_key`.

## 2. Priority order

Queues implement a min-heap order using this lexicographic comparator:

1. smaller `priority_key` wins;
2. if keys tie, earlier (smaller) `arrival_time` wins;
3. if both tie, smaller `patient_id` wins.

Equivalently, the queue orders entries by `(priority_key, arrival_time, patient_id)` ascending. This comparator is mandatory for A, B, and O; heap shape does not matter.

## 3. Required operations

The C++ interface is `ITriageQueue`. An operation returns `std::nullopt` on success when no value is returned. A failed operation must not change the logical set of live patients.

| Operation | Signature | Success result | Error result |
| --- | --- | --- |
| Insert | `optional<QueueError> insert(const QueueEntry&)` | `nullopt` | `duplicate_id` |
| Extract minimum | `Result<QueueEntry> extractMin()` | lowest ordered live entry | `empty_queue` |
| Decrease key | `optional<QueueError> decreaseKey(PatientId, PriorityKey new_key)` | `nullopt` | `missing_id` or `key_not_decreased` |
| Delete patient | `optional<QueueError> deletePatient(PatientId)` | `nullopt` | `missing_id` |
| Build heap | `optional<QueueError> buildHeap(const vector<QueueEntry>&)` | `nullopt` | `duplicate_id` |
| Empty | `bool empty() const` | true iff `size() == 0` | never fails |
| Size | `size_t size() const` | number of live patients | never fails |

### `buildHeap` semantics

`buildHeap` **replaces all existing queue contents atomically**. It validates the complete input first. If any duplicate ID appears in `entries`, it returns `duplicate_id` and leaves the old queue unchanged. On success, the queue contains exactly `entries`.

## 4. Edge cases

- **Duplicate ID on insert:** return `duplicate_id`; retain existing patient unchanged.
- **Duplicate IDs in buildHeap:** return `duplicate_id`; retain the queue exactly as it was.
- **Missing ID:** `decreaseKey` and `deletePatient` return `missing_id`.
- **Extract from empty queue:** return `empty_queue`; do not modify the queue.
- **Attempted increase or unchanged key:** `decreaseKey(id, new_key)` requires `new_key < old_key`. Otherwise return `key_not_decreased` and make no change.
- **Ties:** use the comparator in section 2, never insertion order alone.

## 5. Structural invariants

### A — Indexed binary min-heap

1. The array satisfies heap order under `(priority_key, arrival_time, patient_id)`.
2. Each live `patient_id` occurs exactly once in the heap.
3. The locator contains every live ID and no deleted ID.
4. For every heap index `i`, `locator[heap[i].patient_id] == i`.
5. Every swap, sift-up, sift-down, extraction, and deletion updates the locator before the operation returns.

### B — Lazy-invalidation binary heap

1. Each logical live patient has **exactly one current version** in the version table.
2. Every physical heap record includes `(patient_id, version, priority_key, arrival_time)`.
3. A heap record is current only if its version equals the version table value and the patient is live.
4. Stale records are discarded before an item is returned by `extractMin`; therefore a stale record can never be returned.
5. Logical `size()` counts live patients only, never stale heap records.
6. Compact when `stale_count > live_count`; rebuild using only current records.

### O — Unsorted-array oracle

1. Each live ID occurs exactly once.
2. `extractMin` scans all live entries using the common comparator.
3. It is the reference output used to compare A and B after every simulator event.

## 6. Simulator boundary and event ordering

The simulator, not a queue, owns clinical policy and timestamps.

- It converts severity and waiting history into `priority_key` before every queue call.
- Under linear aging it computes the static key defined by the selected model.
- Under step-wise aging it schedules escalation events and calls `decreaseKey` only when a patient’s new key is smaller.
- It sends the same operation, with the same entry/key values, to A, B, and O.
- It compares results and live sizes after every event; a difference is a correctness failure.

When events share the same timestamp, process them in this exact order:

1. treatment allocation (`extractMin`),
2. patient departure (`deletePatient`),
3. severity revision / scheduled escalation (`decreaseKey`),
4. patient arrival (`insert`).

Thus, if a treatment and escalation have the same timestamp, treatment runs first; the escalated patient affects later events only.

## 7. Tiny cross-implementation example

Start with an empty queue. For each row, A, B, and O must produce the expected result.

| Step | Call | Expected result | Live order after call |
| --- | --- | --- | --- |
| 1 | `insert({20, 5, 100})` | success | `[20]` |
| 2 | `insert({10, 5, 90})` | success | `[10, 20]` because arrival 90 is earlier |
| 3 | `insert({5, 5, 90})` | success | `[5, 10, 20]` because smaller ID breaks exact tie |
| 4 | `decreaseKey(20, 3)` | success | `[20, 5, 10]` |
| 5 | `extractMin()` | `{patient_id: 20, priority_key: 3, arrival_time: 100}` | `[5, 10]` |
| 6 | `deletePatient(5)` | success | `[10]` |
| 7 | `decreaseKey(10, 5)` | `key_not_decreased` | `[10]` unchanged |
| 8 | `extractMin()` | `{patient_id: 10, priority_key: 5, arrival_time: 90}` | `[]` |
| 9 | `extractMin()` | `empty_queue` | `[]` |

Expected extracted patient-ID sequence: **`20, 10`**.
