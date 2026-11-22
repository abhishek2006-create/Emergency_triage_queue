# D6 - Analyst: Cost Model and Measurement Analysis

Author: Harshwardhan Mukhedkar (250041017).

**Project:** Emergency Triage Queue
**Role:** D6 - Analyst
**Status:** Revised analytical note
**Scope:** Structures A and B, revision-share benchmark, and aging-policy cost considerations

## 1. Purpose and scope

This note develops an operation-count cost model for the two priority-queue implementations and explains how to interpret the existing analyst benchmark. It distinguishes theoretical expectations from measured results and records limitations that affect the strength of any performance conclusion.

The existing `analyst/benchmark.cpp` benchmark varies revision share from 0% to 90% in 10-percentage-point increments and runs five repetitions for each share and each structure. It times application of a pre-generated operation sequence. It does **not** independently compare the linear-aging and step-wise-aging simulator policies. Those policies therefore cannot be ranked from `results/analyst.csv`.

## 2. Notation and assumptions

Let:

- \(L\): number of live patients in the queue.
- \(H\): number of physical heap nodes.
- \(S=H-L\): number of stale nodes in Structure B.
- \(C\): capacity of Structure A's locator hash table.
- \(I\): number of inserts; \(X\): number of extract-min operations.
- \(R\): number of decrease-key/revision operations.
- \(D\): number of deletions.
- \(B\): number of items supplied to build-heap.
- \(U\): number of explicit priority updates caused by aging, if the simulator implements aging by updating queue keys.

The logarithm in heap bounds is base 2; its base does not affect asymptotic complexity.

Expected-time hash-table bounds assume a reasonable distribution of identifiers and the implementation's controlled load factor. They are not worst-case guarantees. A pathological probe sequence can make an individual hash operation \(O(C)\).

## 3. Structure A: indexed binary min-heap

Structure A stores one heap entry per live patient and maintains a locator table mapping patient ID to heap index. Its locator uses open addressing with linear probing.

### 3.1 Locator-table behavior

The table starts with capacity 16. Before a new insertion, it checks the combined occupied-plus-deleted load. At a 70% threshold, it either doubles the capacity when occupied entries alone are near the threshold, or rehashes at the same capacity to remove tombstones. Rehashing scans the old table and reinserts occupied entries.

Consequences:

- `contains`, `find`, and `erase` take expected \(O(1)\) time under the hash-distribution/load assumptions.
- `insert` takes expected amortized \(O(1)\) time; an occasional rehash costs \(O(C)\).
- In the worst case, linear probing can inspect \(O(C)\) slots.

### 3.2 Operation costs

A heap insertion appends one entry and sifts it upward. Extract-min and deletion move the final entry into the removed position and restore heap order by sifting up or down. Decrease-key changes the key and sifts upward. Each sift traverses at most the heap height.

| Operation | Expected / amortized cost |
|---|---:|
| Insert | \(O(\log L)\) |
| Extract-min | \(O(\log L)\) |
| Decrease-key | \(O(\log L)\) |
| Delete by ID | \(O(\log L)\) |
| Build-heap | \(O(B)\) expected |

The first four bounds include expected/amortized constant-time locator work. Build-heap includes expected hash-table construction plus bottom-up heapification, which is linear.

A more explicit expression for a sequence of operations is:

\[
T_A = O((I+X+R+D)\log(L_{\max}+1) + B)
\]

where \(L_{\max}\) is the maximum live queue size during the sequence. This is an upper-bound model, not a prediction of elapsed nanoseconds.

## 4. Structure B: lazy-invalidation heap

Structure B stores heap nodes with patient IDs and versions. The active-state map stores the current version for each live patient. A node is valid only when its version matches the active map.

A revision inserts a new node with a fresh version; the older node becomes stale. A deletion removes the patient from the active map and inserts an invalid marker. Extract-min discards stale roots until it reaches a valid root or the heap becomes empty.

### 4.1 Physical-size invariant and compaction

After public operations complete, the implementation calls `compactIfNeeded()` where applicable. Compaction occurs when stale nodes exceed live patients:

\[
S > L.
\]

After compaction, the stale count is zero. At operation boundaries, the maintained invariant is:

\[
S \leq L,\qquad H=L+S\leq 2L.
\]

When \(L=0\), any stale nodes trigger compaction, leaving an empty physical heap. Thus the physical heap is bounded by twice the live population at completed public-operation boundaries.

Compaction filters valid nodes and heapifies the retained nodes. Its cost is \(O(H)\). At the trigger point \(S>L\), we have \(H=L+S<2S\), so the scan/rebuild cost is \(O(S)\). The stale nodes that caused the threshold crossing can therefore pay for the rebuild: compaction contributes \(O(1)\) amortized work per stale node created, in addition to the heap work for creating or removing nodes.

The bound is about completed public-operation boundaries. Temporary internal states during an operation need not satisfy it.

### 4.2 Operation costs

Let \(H\) be the physical heap size immediately before an operation.

| Operation | Cost |
|---|---:|
| Insert | Expected \(O(1)+O(\log(H+1))\) |
| Decrease-key / revision | Expected \(O(1)+O(\log(H+1))\), plus amortized compaction |
| Delete | Expected \(O(1)+O(\log(H+1))\), plus amortized compaction |
| Extract-min | \(O((k+1)\log(H+1))\) for \(k\) stale roots discarded in that call; stale cleanup is amortized over the lifetime of the nodes |
| Build-heap | Expected \(O(B)\) |

The extract-min worst case for a single call can be larger than \(O(\log H)\), because it may discard many stale roots. Each stale node, however, is removed at most once. Across a sequence, stale-root cleanup is chargeable to the operations that created stale nodes. Compaction is also amortized as described above.

With \(H\leq 2L\) at completed operation boundaries, the logarithmic heap factor is \(O(\log(L+1))\). The constant factors and actual number of stale nodes can still affect runtime.

## 5. Aging-policy contribution

The analyst benchmark currently measures revision-share workloads; it does not provide a separate controlled comparison of linear aging versus step-wise aging.

If the simulator implements aging by issuing \(U\) explicit priority changes to the queue, the additional heap work is approximately:

- Structure A: \(O(U\log(L_{\max}+1))\).
- Structure B: \(O(U\log(H_{\max}+1))\) heap insertions, with stale cleanup and compaction accounted for amortized.

If the simulator instead computes effective priority during selection or scans a waiting-patient registry to identify step thresholds, that work is not represented by the explicit-update formula. It must be modeled from the actual simulator path: for example, a scan over \(W\) waiting patients costs \(O(W)\) per scan, and the total depends on how many scans are performed.

Therefore, do not infer an aging-policy winner from the existing analyst CSV. Report the simulator's actual aging mechanism and benchmark it separately if the project requires a direct policy comparison.

## 6. Measurement method in the existing benchmark

The current `analyst/benchmark.cpp`:

1. Uses revision shares of 0%, 10%, ..., 90%.
2. Runs five repetitions per share and per structure.
3. Generates a deterministic operation sequence from the supplied seed, share, and repetition.
4. Generates the operation sequence before starting the timer.
5. Times the application of the sequence using `std::chrono::steady_clock`.
6. Records attempted operations, successful operations, elapsed nanoseconds, nanoseconds per attempted operation, and a peak-memory field.
7. Calls `validate()` after the timed region.

The reported `ns_per_attempted_operation` is calculated as elapsed nanoseconds divided by the number of attempted operations. It is not a per-successful-operation metric. In the supplied CSV all attempts were successful, so the two denominators happen to match for those rows.

The A and B workloads use the same seed formula for a given share and repetition. This is useful for a paired workload comparison, but the benchmark does not incorporate a structure label into the seed.

## 7. Measurement limitations

The current measurements should be treated as an initial comparison, not definitive evidence of a universal winner.

- **Short timed workload:** 20,000 operations per run produce elapsed times in the low-millisecond range. The recorded values are often quantized at approximately millisecond granularity, which limits resolution.
- **Memory attribution:** the benchmark's peak-working-set field is process-level peak memory. Since A and B execute sequentially in the same process, the value is cumulative and cannot be interpreted as an isolated per-structure peak.
- **Aging coverage:** the benchmark varies revision share but does not independently vary linear and step-wise aging.
- **Workload scope:** results depend on the generated operation mix, queue sizes, identifier distribution, and seed. They do not prove one structure is faster for every workload.
- **Expected hash behavior:** the theoretical expected \(O(1)\) locator/map assumptions depend on hash distribution and load behavior; they are not worst-case guarantees.
- **Validation timing:** validation is performed after the timed region, so validation cost is not included in the reported operation time.

For a stronger follow-up experiment, increase the operation count enough to produce stable timing intervals, retain at least five repetitions, report mean and spread (or confidence intervals), and isolate each structure's memory measurement in a separate process or otherwise reset/measure peak memory independently. Keep the workload and compiler/build configuration fixed across paired runs.

## 8. How to report the comparison

Use the following interpretation:

- Structure A maintains one physical heap entry per live patient and pays for locator maintenance during swaps and removals.
- Structure B simplifies revisions by inserting fresh versioned nodes, but pays for stale-node validation, cleanup, and occasional compaction.
- The Structure B invariant bounds physical heap size by \(2L\) at completed public-operation boundaries under the current compaction rule.
- The asymptotic heap-operation bounds are logarithmic for both structures, while build-heap is linear.
- The measured result is workload-specific. The current benchmark is not sufficient to claim a universal winner or to compare the two aging policies.

## 9. Chronology and reproducibility note

The repository history shows that commit `1086d24` added the original `docs/derivations/D6_Analyst.md`, `analyst/benchmark.cpp`, and `results/analyst.csv` together. Consequently, the repository does not establish that the original D6 model was committed before the measurements it discusses.

This revised note is a more complete analytical explanation written after reviewing the implementation and benchmark. It must not be represented as a pre-measurement prediction. Preserve the original commit history and describe this revision honestly in the report/README if chronology is discussed.

## 10. Conclusion

Both structures provide logarithmic heap work for their principal queue operations. Structure A uses an indexed heap and expected/amortized constant-time locator operations. Structure B trades direct updates for versioned insertions and stale-node management; its compaction rule keeps the physical heap within twice the live population at completed operation boundaries. The current analyst benchmark offers an initial revision-share comparison, but its short runs, process-level memory measurement, and lack of separate aging-policy experiments limit the conclusions that can responsibly be drawn.
