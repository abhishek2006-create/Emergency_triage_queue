EMERGENCY TRIAGE QUEUE — SYNTHETIC BENCHMARK DATABASE
Version: 1.0
Generated: 2026-11-21
Seed: 20261121

CONTENTS
- emergency_triage_synthetic.db : SQLite database
- triage_patients.csv           : 10,000 synthetic patient rows
- triage_events.csv             : 20,000 synthetic queue-operation rows

PROJECT FIT
This dataset is designed for the Emergency Triage Queue comparison:
A) Indexed min-heap + locator hash table
B) Lazy-invalidation heap

The ordering rule matches the project:
(priority_key ASC, arrival_time_utc ASC, patient_id ASC)

The event trace contains:
- 10,000 INSERT operations
- 6,000 REVISION operations
- 2,000 TREAT operations
- 2,000 DEPART operations

The simulation window is 30 days. All 10,000 arrivals occur during the
first 12 hours; subsequent revision/treatment/departure events occur over
the remainder of the simulated window. Event IDs follow chronological event
time, and all revision/removal events refer to patients active at that point.

IMPORTANT INTERPRETATION
This is a synthetic software-testing dataset, not a live hospital database.
It contains no real patient records. benchmark_triage_level and priority_key
are artificial workload labels, not clinically validated triage assessments.
Do not use these fields for diagnosis, treatment, or real patient prioritization.

DATABASE TABLES
patients:
  patient_id, age_years, sex_code, benchmark_triage_level,
  initial_priority_key, arrival_time_utc, initial_version

queue_events:
  event_id, patient_id, operation, event_time_utc,
  old_priority_key, new_priority_key, version, notes

dataset_metadata:
  key, value

SUGGESTED EXPERIMENTS
1. Load the same ordered event trace into both implementations.
2. Compare correctness after each event, not only final queue contents.
3. Measure data-structure operation time separately from SQLite query time.
4. Run repeated trials with the same seed and identical event order.
5. Vary revision share and stale-entry fraction in separate controlled runs.
6. If testing concurrent access, add a synchronization/transaction layer and
   report it separately from the single-threaded heap benchmark.

REPRODUCIBILITY
The database and CSVs were generated deterministically with seed 20261121.
The exact generated dataset is included in this bundle. The synthetic queue
trace is custom-generated for this project; it is NOT a downloaded Synthea
patient dataset.

REFERENCES / ATTRIBUTION
[1] Walonoski, J., Kramer, M., Nichols, J., Quina, A., Moesel, C., Hall, D.,
    Duffett, C., Dube, K., Gallagher, T., & McLachlan, S. (2018).
    Synthea: An approach, method, and software mechanism for generating
    synthetic patients and the synthetic electronic health care record.
    Journal of the American Medical Informatics Association, 25(3), 230–238.
    https://doi.org/10.1093/jamia/ocx079

[2] Synthea — Synthetic Patient Population Simulator.
    https://github.com/synthetichealth/synthea
    Synthea is referenced as a public synthetic-health-data generator and
    background source. The rows in this custom queue benchmark database
    were generated specifically for this project, not copied from Synthea.

[3] Synthea Downloads and sample-data documentation.
    https://synthetichealth.github.io/downloads.html

Suggested report wording:
"We created a deterministic synthetic emergency-queue workload informed by
the use of synthetic patient records in Synthea (Walonoski et al., 2018).
The benchmark dataset contains 10,000 synthetic patient entries and 20,000
queue events over a 30-day simulated period. Priority keys are artificial
benchmark values and are not clinical triage recommendations."
