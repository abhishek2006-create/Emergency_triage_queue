UPLOADED SQLITE DATASET — REPLAY NOTES

File: emergency_triage_synthetic.db
Included in the repository under data/.
The uploaded bundle also supplied triage_patients.csv and triage_events.csv.

Contents reported by dataset_metadata:
- 10,000 synthetic patient records
- 20,000 queue events
- 10,000 INSERT, 6,000 REVISION, 2,000 TREAT, 2,000 DEPART
- generated with seed 20261121
- simulation window: 30 days

Schema:
- patients(patient_id TEXT, age_years, sex_code, benchmark_triage_level,
  initial_priority_key, arrival_time_utc, initial_version)
- queue_events(event_id, patient_id, operation, event_time_utc,
  old_priority_key, new_priority_key, version, notes)
- dataset_metadata(key, value)

The replay adapter also supports the project's original v2 schema
(arrival_minute, event_seq, event_time_minute). Text patient IDs such as
P000123 are mapped to their numeric suffix for the queue's uint64_t ID type.
ISO-8601 timestamps are normalized to UTC seconds and used for arrival ordering.

Important operation semantics for this uploaded trace:
- INSERT adds the patient with the initial priority key.
- REVISION updates the active priority. A lower key uses decreaseKey;
  a higher key is represented by delete + reinsert, preserving arrival time,
  because the shared queue API intentionally exposes decreaseKey only.
- TREAT removes the specific patient named by the trace (deletePatient).
  This dataset's TREAT rows are explicit patient-removal events; they are not
  assertions that extractMin must return that patient.
- DEPART removes the named patient.

The original project v2 database has a different contract: its TREAT events
record the expected extractMin result. The adapter preserves that behavior for
that schema. Both datasets are tested independently by CTest.

Provenance and interpretation:
The uploaded bundle's own README identifies these rows as a custom-generated
synthetic queue workload and explicitly states that they were not copied from
Synthea. Synthea is cited there as background/reference material only. Do not
describe these particular database rows as downloaded Synthea records.
All priority values and triage labels are artificial benchmark values, not
clinically validated assessments. This project is a data-structure experiment,
not a clinical decision-support system.
