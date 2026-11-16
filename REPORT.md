# Emergency Triage Queue Integration Report

## Delivered

This integration adds a deterministic 30-day, event-driven replay, linear and 30-minute stepwise aging, per-severity waiting-time summaries, and a seeded A/B benchmark with revision shares from 0% to 90%, five repetitions, operation timing, and OS peak-memory observations. D6 predeclares the operation-count model and explains measurement limits. The original A, B, Oracle, Adversary, and D1–D5 artifacts remain in their original directories.

## Simulator semantics

Time is integer minutes. Patients arrive according to a fixed seeded synthetic stream. Service is single-server and non-preemptive. At equal timestamps, arrivals are inserted, aging thresholds due at that minute are applied, a completed service is removed, and the next service begins. Linear aging uses the static key `-severity*1,000,000 + 100*arrival_minute`, equivalent in ordering to a common 100 urgency units/minute. Stepwise aging lowers key by 200 at each 30-minute waiting threshold. The selected 30-day run is illustrative and not clinical evidence.

## Test status

The user ran `scripts\verify.ps1` from Command Prompt with the working compiler. Confirmed passes: 100,000-operation integration/tie-breaking; one-million-operation A/B/Oracle differential with seed 20261116; one-million-element construction/extraction; and all one-million-scale adversarial scenarios with seed 20261116. Adversarial counters included 17,951,445 sift-up swaps/comparisons, 34,912,261 shuffled-extraction comparisons, and 19 Structure B compactions in the revision workload.

The sanitizer build did not run: MinGW GCC 6.3.0 reported an internal compiler error in its `<random>` header (`pretty-print.c:630`), so this is an unsupported/failed compiler build, not a sanitizer finding. The run continued. The simulator completed 30 days with seed 20261116: 6,605 arrivals, 6,604 patients treated by the horizon, 17 step escalations. Waiting-time CSV summaries are for treated patients only; the one patient still waiting at the horizon is right-censored and excluded from those wait summaries. The benchmark completed 100 run-level rows (two structures × ten revision shares × five repetitions), with 20,000/20,000 operations successful in every row. Because observed elapsed times are only about 2–7 ms and appear quantized at millisecond scale, treat them as a smoke benchmark rather than reliable comparative performance evidence. Peak resident memory is process-level and cumulative as described in D6. No teammate PR was supplied, so no teammate review is claimed.

## Limitations found

- The queue comparator orders lower numeric keys first; application code must consistently map higher urgency to lower keys.
- The stepwise simulator scans the waiting-patient registry to find upcoming aging thresholds, so simulation orchestration is O(number of waiting patients) per event even though queue updates are logarithmic.
- The implementation accepts only severity improvement revisions because the common queue API exposes decrease-key, not arbitrary priority increases.
- The generated arrival/service distributions are synthetic. Neither the simulator nor benchmarks establish clinical efficacy, starvation freedom, nor safe triage decisions.
- Peak resident memory is process-level and cumulative, not a precise per-structure allocation measurement.
- Published ED operational studies use different outcomes and assumptions; see D6. No direct numerical comparison is warranted.

## Integration review

The added code uses the common `QueueEntry`, comparator direction, and public methods from the existing A/B interfaces. It builds a common seeded workload for both structures, keeps the operation trace fixed across them, and writes raw run-level results. The simulator uses explicit event timestamps rather than minute-by-minute polling and processes due aging before a simultaneous service departure, matching D3's stated tie convention. Source review also caught and corrected an initial benchmark repetition/accounting problem and a simulator success check that had included arrivals beyond the horizon.

The review remains a source-level integration review because the local compiler could not start. It is not an independent teammate review, and runtime behavior of the new code remains unverified until the verification script can run on a working compiler installation.
