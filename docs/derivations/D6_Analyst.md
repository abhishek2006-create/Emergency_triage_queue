# D6: Benchmark Cost Model and Measurement Protocol

## Predeclared operation-cost model

For workload W, define C6(W) = I log2(n+1) + X log2(n+1) + R log2(n+1) + D log2(n+1) + B n, where I, X, R, D, and B are insert, extract, revision (decrease-key), delete, and bulk-build counts, and n is the maximum live-patient population. This is an asymptotic comparison model, not a runtime prediction. It assumes expected O(1) identifier lookup for A's locator and B's hash table; B revision/deletion costs are amortized and include periodic compaction. Oracle extract is O(n), so its corresponding term is X*n. Fixed overhead, cache effects, allocator behavior, and hash-table worst cases are outside this model.

The benchmark records operation mix and observed wall-clock time separately. It does not fit constants after seeing results. Revision share is defined as revision operations divided by all attempted operations, from 0% through 90% in 10-point increments. A deterministic seed is derived from the declared base seed, share, repetition, and structure label. Five repetitions per configuration are recorded individually.

## Measurement and interpretation

Timing uses `std::chrono::steady_clock`; the output records elapsed nanoseconds and nanoseconds per successfully applied operation. Peak memory is process peak resident working set on Windows or peak resident set size on Unix-like systems, sampled after each run. Because the program is single-process and cumulative peak RSS cannot be reset portably, peak-memory rows are upper bounds for that process run, not isolated per-queue allocations. Repeat a single configuration in separate processes for isolated OS-level peaks.

Use optimized builds and report compiler, version, flags, OS, CPU, and seed with any submitted figures. The supplied CSV is raw run-level data; summarize median and range/quantiles externally. Five repetitions are small and are not a formal inferential sample. In the recorded local run, each 20,000-operation sample took only about 2–7 ms and timing values were visibly quantized at roughly millisecond increments; these measurements are too short for a strong A/B speed claim. Increase the workload substantially and repeat on a calibrated, otherwise idle machine before drawing performance conclusions. The deterministic generated workload is synthetic, has no clinical calibration, and cannot establish clinical safety or waiting-time guarantees.

## Published-work comparison

The project measures implementation costs of two binary-heap designs on synthetic operation traces. This differs from clinical/operational triage studies. For example, Chen et al., “Managing responsiveness in the emergency department: Comparing dynamic priority queue with fast track” (Journal of Operations Management, 2018), evaluates emergency department operational policies and reports average length-of-stay effects using a queueing/operations model. Its outcomes and assumptions are not directly comparable to nanoseconds per data-structure operation. The paper supports studying waiting-time-dependent prioritization as an operational policy; it does not validate this project's severity scale, aging constants, simulator arrivals, or queue implementation. No numerical head-to-head claim is made.

Reference: https://doi.org/10.1016/j.jom.2018.03.001
