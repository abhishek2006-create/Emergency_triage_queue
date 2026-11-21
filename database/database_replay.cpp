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
};

void checkSql(int rc, sqlite3* db, const std::string& where) {
    if (rc != SQLITE_OK && rc != SQLITE_DONE && rc != SQLITE_ROW) {
        throw std::runtime_error(where + ": " + (db ? sqlite3_errmsg(db) : "SQLite error"));
    }
}

Dataset loadDataset(const std::string& path, std::uint64_t& elapsedNs) {
    const auto start = std::chrono::steady_clock::now();
    sqlite3* db = 0;
    checkSql(sqlite3_open_v2(path.c_str(), &db, SQLITE_OPEN_READONLY, 0), db, "open database");
    Dataset data;

    sqlite3_stmt* patientStmt = 0;
    const char* patientSql =
        "SELECT patient_id, arrival_minute, initial_priority_key "
        "FROM patients ORDER BY patient_id";
    checkSql(sqlite3_prepare_v2(db, patientSql, -1, &patientStmt, 0), db, "prepare patients");
    int rc = SQLITE_OK;
    while ((rc = sqlite3_step(patientStmt)) == SQLITE_ROW) {
        PatientRow p;
        p.id = static_cast<triage::PatientId>(sqlite3_column_int64(patientStmt, 0));
        p.arrival = static_cast<triage::Timestamp>(sqlite3_column_int64(patientStmt, 1));
        p.initial_key = static_cast<triage::PriorityKey>(sqlite3_column_int64(patientStmt, 2));
        if (!data.patients.insert(std::make_pair(p.id, p)).second)
            throw std::runtime_error("duplicate patient_id in patients table");
    }
    checkSql(rc, db, "read patients");
    sqlite3_finalize(patientStmt);

    sqlite3_stmt* eventStmt = 0;
    const char* eventSql =
        "SELECT event_seq, event_time_minute, patient_id, operation, "
        "COALESCE(old_priority_key, 0), COALESCE(new_priority_key, 0) "
        "FROM queue_events ORDER BY event_seq";
    checkSql(sqlite3_prepare_v2(db, eventSql, -1, &eventStmt, 0), db, "prepare events");
    while ((rc = sqlite3_step(eventStmt)) == SQLITE_ROW) {
        ReplayEvent e;
        e.seq = sqlite3_column_int64(eventStmt, 0);
        e.event_minute = sqlite3_column_int64(eventStmt, 1);
        e.patient_id = static_cast<triage::PatientId>(sqlite3_column_int64(eventStmt, 2));
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
                throw std::runtime_error("event_time_minute is not chronological");
            data.events.push_back(e);
        } else {
            throw std::runtime_error("event_seq is not strictly increasing");
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
            if (current->second != e.old_key || e.new_key >= e.old_key)
                throw std::runtime_error("revision old/new key mismatch in source trace");
            err = q.decreaseKey(e.patient_id, e.new_key);
            if (err == triage::QueueError::none) current->second = e.new_key;
        } else if (e.op == Op::Treat) {
            std::map<triage::PatientId, triage::PriorityKey>::iterator current = activeKeys.find(e.patient_id);
            if (current == activeKeys.end() || current->second != e.old_key)
                throw std::runtime_error("TREAT expected patient/key does not match active replay state");
            triage::ExtractResult result = q.extractMin();
            if (!result.ok)
                throw std::runtime_error(std::string(label) + ": unexpected empty queue on TREAT");
            if (result.entry.patient_id != e.patient_id || result.entry.priority_key != e.old_key)
                throw std::runtime_error(std::string(label) + ": extractMin disagrees with expected TREAT patient/key");
            activeKeys.erase(current);
            ++successful;
            if (q.size() != activeKeys.size())
                throw std::runtime_error(std::string(label) + ": live size mismatch after TREAT");
            if ((i + 1) % 1000 == 0 && !q.validate())
                throw std::runtime_error(std::string(label) + ": invariant validation failed");
            continue;
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
