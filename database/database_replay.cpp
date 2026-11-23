#include "../src/structA/indexed_heap.hpp"
#include "../src/structB/lazy_heap.hpp"
#include "../oracle/triage_oracle.hpp"

#include <sqlite3.h>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <ctime>
#include <cstdio>
#include <cctype>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct PatientRow {
    triage::PatientId id;
    triage::Timestamp arrival;
    triage::PriorityKey initial_key;
};

enum class Op { Insert, Revision, Treat, Depart };

struct ReplayEvent {
    std::int64_t seq;
    std::int64_t event_minute;
    triage::PatientId patient_id;
    Op op;
    triage::PriorityKey old_key;
    triage::PriorityKey new_key;
};

struct Dataset {
    std::map<triage::PatientId, PatientRow> patients;
    std::vector<ReplayEvent> events;
    bool treatById;
};

void checkSql(int rc, sqlite3* db, const std::string& where) {
    if (rc != SQLITE_OK && rc != SQLITE_DONE && rc != SQLITE_ROW) {
        throw std::runtime_error(where + ": " + (db ? sqlite3_errmsg(db) : "SQLite error"));
    }
}


bool hasColumn(sqlite3* db, const char* table, const char* column) {
    const std::string sql = std::string("PRAGMA table_info(") + table + ")";
    sqlite3_stmt* stmt = 0;
    checkSql(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0), db, "inspect schema");
    bool found = false;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* name = sqlite3_column_text(stmt, 1);
        if (name && std::string(reinterpret_cast<const char*>(name)) == column) {
            found = true;
            break;
        }
    }
    sqlite3_finalize(stmt);
    return found;
}

triage::PatientId parsePatientId(const unsigned char* raw) {
    if (!raw) throw std::runtime_error("NULL patient_id");
    const std::string value(reinterpret_cast<const char*>(raw));
    std::size_t pos = 0;
    while (pos < value.size() && !std::isdigit(static_cast<unsigned char>(value[pos]))) ++pos;
    if (pos == value.size()) throw std::runtime_error("patient_id has no numeric component: " + value);
    std::uint64_t id = 0;
    for (; pos < value.size(); ++pos) {
        if (!std::isdigit(static_cast<unsigned char>(value[pos])))
            throw std::runtime_error("patient_id has a non-numeric suffix: " + value);
        const unsigned digit = static_cast<unsigned>(value[pos] - '0');
        if (id > (std::numeric_limits<std::uint64_t>::max() - digit) / 10)
            throw std::runtime_error("patient_id is out of range: " + value);
        id = id * 10 + digit;
    }
    if (id == 0) throw std::runtime_error("patient_id must map to a positive integer: " + value);
    return static_cast<triage::PatientId>(id);
}

triage::Timestamp parseUtcTimestamp(const unsigned char* raw) {
    if (!raw) throw std::runtime_error("NULL timestamp");
    const std::string value(reinterpret_cast<const char*>(raw));
    int year=0, month=0, day=0, hour=0, minute=0, second=0;
    if (std::sscanf(value.c_str(), "%4d-%2d-%2dT%2d:%2d:%2d",
                    &year, &month, &day, &hour, &minute, &second) != 6)
        throw std::runtime_error("unsupported ISO-8601 timestamp: " + value);
    // Convert the civil date to days since 1970-01-01 (UTC).
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(year - era * 400);
    const unsigned mp = static_cast<unsigned>(month + (month > 2 ? -3 : 9));
    const unsigned doy = (153 * mp + 2) / 5 + static_cast<unsigned>(day) - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const long long days = static_cast<long long>(era) * 146097 + doe - 719468;
    long long seconds = days * 86400 + hour * 3600 + minute * 60 + second;
    // Apply an optional ISO-8601 timezone offset.
    std::size_t zone = value.find_last_of("+-");
    if (zone != std::string::npos && zone > 10 && value.size() >= zone + 6) {
        int zh=0, zm=0;
        if (std::sscanf(value.c_str()+zone+1, "%2d:%2d", &zh, &zm) == 2) {
            const int offset = (zh * 60 + zm) * 60;
            seconds += value[zone] == '+' ? -offset : offset;
        }
    }
    return static_cast<triage::Timestamp>(seconds);
}

Dataset loadDataset(const std::string& path, std::uint64_t& elapsedNs) {
    const auto start = std::chrono::steady_clock::now();
    sqlite3* db = 0;
    checkSql(sqlite3_open_v2(path.c_str(), &db, SQLITE_OPEN_READONLY, 0), db, "open database");
    Dataset data;
    const bool patientMinutes = hasColumn(db, "patients", "arrival_minute");
    const bool eventMinutes = hasColumn(db, "queue_events", "event_time_minute");
    data.treatById = !eventMinutes;
    const bool hasEventSeq = hasColumn(db, "queue_events", "event_seq");
    const char* patientArrivalColumn = patientMinutes ? "arrival_minute" : "arrival_time_utc";
    const char* eventTimeColumn = eventMinutes ? "event_time_minute" : "event_time_utc";
    const char* eventSeqColumn = hasEventSeq ? "event_seq" : "event_id";
    if (!hasColumn(db, "patients", patientArrivalColumn) ||
        !hasColumn(db, "patients", "initial_priority_key") ||
        !hasColumn(db, "queue_events", eventTimeColumn) ||
        !hasColumn(db, "queue_events", eventSeqColumn))
        throw std::runtime_error("unsupported SQLite schema: expected project v2 or uploaded dataset columns");

    std::map<std::string, triage::PatientId> idMap;
    sqlite3_stmt* patientStmt = 0;
    const std::string patientSql = std::string("SELECT patient_id, ") + patientArrivalColumn +
        ", initial_priority_key FROM patients ORDER BY patient_id";
    checkSql(sqlite3_prepare_v2(db, patientSql.c_str(), -1, &patientStmt, 0), db, "prepare patients");
    int rc = SQLITE_OK;
    while ((rc = sqlite3_step(patientStmt)) == SQLITE_ROW) {
        const unsigned char* rawId = sqlite3_column_text(patientStmt, 0);
        if (!rawId) throw std::runtime_error("NULL patient_id in patients table");
        const std::string externalId(reinterpret_cast<const char*>(rawId));
        const triage::PatientId id = parsePatientId(rawId);
        triage::Timestamp arrival = patientMinutes
            ? static_cast<triage::Timestamp>(sqlite3_column_int64(patientStmt, 1))
            : parseUtcTimestamp(sqlite3_column_text(patientStmt, 1));
        PatientRow patient = {id, arrival, static_cast<triage::PriorityKey>(sqlite3_column_int64(patientStmt, 2))};
        if (!idMap.insert(std::make_pair(externalId, id)).second ||
            !data.patients.insert(std::make_pair(id, patient)).second)
            throw std::runtime_error("duplicate patient identifier after numeric mapping: " + externalId);
    }
    checkSql(rc, db, "read patients");
    sqlite3_finalize(patientStmt);

    sqlite3_stmt* eventStmt = 0;
    const std::string eventSql = std::string("SELECT ") + eventSeqColumn +
        ", " + eventTimeColumn + ", patient_id, operation, "
        "COALESCE(old_priority_key, 0), COALESCE(new_priority_key, 0) "
        "FROM queue_events ORDER BY " + eventSeqColumn;
    checkSql(sqlite3_prepare_v2(db, eventSql.c_str(), -1, &eventStmt, 0), db, "prepare events");
    while ((rc = sqlite3_step(eventStmt)) == SQLITE_ROW) {
        ReplayEvent e;
        e.seq = sqlite3_column_int64(eventStmt, 0);
        e.event_minute = eventMinutes
            ? static_cast<triage::Timestamp>(sqlite3_column_int64(eventStmt, 1))
            : parseUtcTimestamp(sqlite3_column_text(eventStmt, 1));
        const unsigned char* rawId = sqlite3_column_text(eventStmt, 2);
        if (!rawId) throw std::runtime_error("NULL patient_id in queue_events");
        const std::string externalId(reinterpret_cast<const char*>(rawId));
        std::map<std::string, triage::PatientId>::const_iterator mapped = idMap.find(externalId);
        if (mapped == idMap.end())
            throw std::runtime_error("queue event references unknown patient_id: " + externalId);
        e.patient_id = mapped->second;
        const unsigned char* opText = sqlite3_column_text(eventStmt, 3);
        const std::string op = opText ? reinterpret_cast<const char*>(opText) : "";
        if (op == "INSERT") e.op = Op::Insert;
        else if (op == "REVISION") e.op = Op::Revision;
        else if (op == "TREAT") e.op = Op::Treat;
        else if (op == "DEPART") e.op = Op::Depart;
        else throw std::runtime_error("unknown operation in queue_events: " + op);
        e.old_key = static_cast<triage::PriorityKey>(sqlite3_column_int64(eventStmt, 4));
        e.new_key = static_cast<triage::PriorityKey>(sqlite3_column_int64(eventStmt, 5));
        if (data.events.empty() || e.seq > data.events.back().seq) {
            if (!data.events.empty() && e.event_minute < data.events.back().event_minute)
                throw std::runtime_error("event timestamps are not chronological");
            data.events.push_back(e);
        } else {
            throw std::runtime_error("event sequence is not strictly increasing");
        }
    }
    checkSql(rc, db, "read events");
    sqlite3_finalize(eventStmt);

    sqlite3_close(db);

    elapsedNs = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - start).count());
    return data;
}

template <typename Queue>
struct ReplayStats {
    std::uint64_t elapsedNs;
    std::size_t successful;
    std::size_t finalSize;
};

template <typename Queue>
ReplayStats<Queue> replay(const Dataset& data, const char* label) {
    Queue q;
    std::map<triage::PatientId, triage::PriorityKey> activeKeys;
    const auto start = std::chrono::steady_clock::now();
    std::size_t successful = 0;

    for (std::size_t i = 0; i < data.events.size(); ++i) {
        const ReplayEvent& e = data.events[i];
        triage::QueueError err = triage::QueueError::none;

        if (e.op == Op::Insert) {
            std::map<triage::PatientId, PatientRow>::const_iterator p = data.patients.find(e.patient_id);
            if (p == data.patients.end())
                throw std::runtime_error(std::string(label) + ": INSERT references missing patient");
            triage::QueueEntry entry = {p->second.id, p->second.initial_key, p->second.arrival};
            err = q.insert(entry);
            if (err == triage::QueueError::none) activeKeys[e.patient_id] = p->second.initial_key;
        } else if (e.op == Op::Revision) {
            std::map<triage::PatientId, triage::PriorityKey>::iterator current = activeKeys.find(e.patient_id);
            if (current == activeKeys.end())
                throw std::runtime_error(std::string(label) + ": REVISION references inactive patient");
            if (current->second != e.old_key)
                throw std::runtime_error("REVISION old key does not match active replay state");
            if (e.new_key < e.old_key) {
                err = q.decreaseKey(e.patient_id, e.new_key);
            } else if (e.new_key > e.old_key) {
                // The shared queue API exposes decreaseKey only. Model an
                // arbitrary priority revision as delete + reinsert, preserving
                // the original arrival timestamp and patient identity.
                std::map<triage::PatientId, PatientRow>::const_iterator patient = data.patients.find(e.patient_id);
                if (patient == data.patients.end())
                    throw std::runtime_error("REVISION references missing patient record");
                err = q.deletePatient(e.patient_id);
                if (err == triage::QueueError::none) {
                    triage::QueueEntry revised = {e.patient_id, e.new_key, patient->second.arrival};
                    err = q.insert(revised);
                }
            }
            if (err == triage::QueueError::none) current->second = e.new_key;
        } else if (e.op == Op::Treat) {
            std::map<triage::PatientId, triage::PriorityKey>::iterator current = activeKeys.find(e.patient_id);
            if (current == activeKeys.end() || current->second != e.old_key)
                throw std::runtime_error("TREAT expected patient/key does not match active replay state");
            if (data.treatById) {
                // This dataset records the patient selected by its external
                // event trace; TREAT is an explicit removal, not an extractMin
                // assertion. The legacy v2 trace records the expected minimum.
                err = q.deletePatient(e.patient_id);
                if (err == triage::QueueError::none) activeKeys.erase(current);
            } else {
                triage::ExtractResult result = q.extractMin();
                if (!result.ok)
                    throw std::runtime_error(std::string(label) + ": unexpected empty queue on TREAT");
                if (result.entry.patient_id != e.patient_id || result.entry.priority_key != e.old_key)
                    throw std::runtime_error(std::string(label) + ": TREAT expected extractMin patient/key mismatch");
                activeKeys.erase(current);
            }
        } else {
            std::map<triage::PatientId, triage::PriorityKey>::iterator current = activeKeys.find(e.patient_id);
            if (current == activeKeys.end() || current->second != e.old_key)
                throw std::runtime_error("DEPART expected patient/key does not match active replay state");
            err = q.deletePatient(e.patient_id);
            if (err == triage::QueueError::none) activeKeys.erase(current);
        }

        if (err != triage::QueueError::none)
            throw std::runtime_error(std::string(label) + ": queue operation failed at event " + std::to_string(e.seq));
        ++successful;
        if (q.size() != activeKeys.size())
            throw std::runtime_error(std::string(label) + ": live size mismatch at event " + std::to_string(e.seq));
        if ((i + 1) % 1000 == 0 && !q.validate())
            throw std::runtime_error(std::string(label) + ": invariant validation failed");
    }

    if (!q.validate() || q.size() != activeKeys.size())
        throw std::runtime_error(std::string(label) + ": final queue validation failed");
    const std::uint64_t elapsed = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - start).count());
    ReplayStats<Queue> stats = {elapsed, successful, q.size()};
    return stats;
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc < 2 || argc > 3) {
            std::cerr << "Usage: database_replay <database.db> [results.csv]\n";
            return 2;
        }
        const std::string dbPath = argv[1];
        const std::string outputPath = argc == 3 ? argv[2] : "database_replay.csv";

        std::uint64_t dbLoadNs = 0;
        Dataset data = loadDataset(dbPath, dbLoadNs);
        if (data.patients.empty() || data.events.empty())
            throw std::runtime_error("database contains no patients or events");

        ReplayStats<triage::IndexedBinaryMinHeap> a = replay<triage::IndexedBinaryMinHeap>(data, "Structure A");
        ReplayStats<triage::LazyBinaryMinHeap> b = replay<triage::LazyBinaryMinHeap>(data, "Structure B");
        ReplayStats<triage::TriageOracle> o = replay<triage::TriageOracle>(data, "Oracle");

        if (a.successful != data.events.size() || b.successful != data.events.size() ||
            o.successful != data.events.size() || a.finalSize != b.finalSize ||
            b.finalSize != o.finalSize)
            throw std::runtime_error("A/B/Oracle final replay summaries disagree");

        std::ofstream csv(outputPath.c_str());
        if (!csv) throw std::runtime_error("cannot open output CSV: " + outputPath);
        csv << "structure,patients,events,db_load_ns,queue_replay_ns,end_to_end_ns,successful_operations,final_live_patients\n";
        csv << "A," << data.patients.size() << ',' << data.events.size() << ',' << dbLoadNs << ','
            << a.elapsedNs << ',' << (dbLoadNs + a.elapsedNs) << ',' << a.successful << ',' << a.finalSize << '\n';
        csv << "B," << data.patients.size() << ',' << data.events.size() << ',' << dbLoadNs << ','
            << b.elapsedNs << ',' << (dbLoadNs + b.elapsedNs) << ',' << b.successful << ',' << b.finalSize << '\n';
        csv << "Oracle," << data.patients.size() << ',' << data.events.size() << ',' << dbLoadNs << ','
            << o.elapsedNs << ',' << (dbLoadNs + o.elapsedNs) << ',' << o.successful << ',' << o.finalSize << '\n';

        std::cout << "PASS database replay\n"
                  << "patients=" << data.patients.size() << " events=" << data.events.size()
                  << " db_load_ns=" << dbLoadNs << '\n'
                  << "A queue_replay_ns=" << a.elapsedNs << " successful=" << a.successful << '\n'
                  << "B queue_replay_ns=" << b.elapsedNs << " successful=" << b.successful << '\n'
                  << "Oracle queue_replay_ns=" << o.elapsedNs << " successful=" << o.successful << '\n'
                  << "final_live_patients=" << a.finalSize << '\n'
                  << "csv=" << outputPath << '\n';
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAIL database replay: " << ex.what() << '\n';
        return 1;
    }
}
