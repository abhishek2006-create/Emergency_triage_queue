PRAGMA foreign_keys=ON;

CREATE TABLE patients (
    patient_id INTEGER PRIMARY KEY CHECK(patient_id > 0),
    age_years INTEGER NOT NULL CHECK(age_years BETWEEN 1 AND 95),
    sex_code TEXT NOT NULL CHECK(sex_code IN ('F','M','X','U')),
    initial_severity INTEGER NOT NULL CHECK(initial_severity BETWEEN 1 AND 10),
    arrival_minute INTEGER NOT NULL CHECK(arrival_minute >= 0),
    initial_priority_key INTEGER NOT NULL,
    UNIQUE(patient_id, arrival_minute)
);

CREATE TABLE queue_events (
    event_seq INTEGER PRIMARY KEY,
    event_time_minute INTEGER NOT NULL CHECK(event_time_minute >= 0),
    patient_id INTEGER NOT NULL REFERENCES patients(patient_id),
    operation TEXT NOT NULL CHECK(operation IN ('INSERT','REVISION','TREAT','DEPART')),
    old_priority_key INTEGER,
    new_priority_key INTEGER,
    old_severity INTEGER,
    new_severity INTEGER
);

CREATE INDEX idx_events_time ON queue_events(event_time_minute, event_seq);
CREATE INDEX idx_events_patient ON queue_events(patient_id);
CREATE INDEX idx_patients_arrival ON patients(arrival_minute, patient_id);

CREATE TABLE dataset_metadata (key TEXT PRIMARY KEY, value TEXT NOT NULL);
