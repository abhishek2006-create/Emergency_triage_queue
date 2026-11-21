EMERGENCY TRIAGE QUEUE — DATABASE REPLAY DATASET v2.0
Generated: 2026-11-21
Seed: 20261121

FILES
- emergency_triage_database_v2.db : SQLite database
- triage_patients_v2.csv           : patient master records
- triage_events_v2.csv             : ordered replay event stream
- triage_schema_v2.sql             : table definitions

SIZE
- 10,000 synthetic patient records
- 20,000 ordered queue events:
  INSERT 10,000
  REVISION 6,000
  TREAT 2,000
  DEPART 2,000

COMPATIBILITY WITH THE CURRENT REPOSITORY
The current queue contract uses:
- PatientId = uint64_t
- PriorityKey = int64_t
- Timestamp = int64_t simulated minutes
- ascending order: (priority_key, arrival_time, patient_id)
- decreaseKey only accepts a strictly smaller key

This v2 dataset therefore uses numeric patient IDs, integer minute timestamps,
the same ascending ordering, and revision events that strictly decrease the
priority key. TREAT rows record the expected extractMin patient ID so the
replay program can verify correctness. DEPART rows call deletePatient.

Priority formula used to initialize records:
priority_key = -severity * 1,000,000 + 100 * arrival_minute

For a REVISION event, severity increases (benchmark urgency improves) and the
new priority key is strictly smaller. This is a synthetic workload convention,
not a clinical scoring system.

RECOMMENDED REPLAY
1. Read queue_events ordered by event_seq.
2. On INSERT, load the corresponding patient row and call insert(QueueEntry).
3. On REVISION, call decreaseKey(patient_id, new_priority_key).
4. On TREAT, call extractMin() and assert returned patient_id equals event.patient_id.
5. On DEPART, call deletePatient(patient_id).
6. After every operation, compare A, B and Oracle results, sizes and invariants.
7. Keep SQLite fetch time separate from queue-operation time.

IMPORTANT
This is a synthetic benchmark database, not a live hospital database and not
a clinical decision-support dataset. It contains no real patient information.
It is not copied from Synthea. It is generated specifically to match the
current queue interface and benchmark operation semantics.

PROVENANCE / CITATION
The dataset rows and event stream were custom-generated for this project with
seed 20261121. Synthea is a relevant background reference for the use of
synthetic electronic health records, but it was not the source of these rows:

Walonoski, J., Kramer, M., Nichols, J., Quina, A., Moesel, C., Hall, D.,
Duffett, C., Dube, K., Gallagher, T., & McLachlan, S. (2018).
Synthea: An approach, method, and software mechanism for generating synthetic
patients and the synthetic electronic health care record.
Journal of the American Medical Informatics Association, 25(3), 230–238.
https://doi.org/10.1093/jamia/ocx079

Official project: https://github.com/synthetichealth/synthea
