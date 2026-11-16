#include "../src/structA/indexed_heap.hpp"
#include "../src/structB/lazy_heap.hpp"
#include <cassert>
#include <iostream>
#include <map>
#include <vector>
#include <random>
#include <limits>
using namespace triage;
int main() {
    IndexedBinaryMinHeap a;
    LazyBinaryMinHeap b;
    std::map<PatientId, QueueEntry> oracle;
    std::mt19937_64 rng(20261116);
    for (int step = 0; step < 1000000; ++step) {
        int op = static_cast<int>(rng() % 5);
        PatientId id = rng() % 400;
        QueueError expected = QueueError::none;
        if (op == 0) {
            QueueEntry e = {id, static_cast<PriorityKey>(rng() % 10000) - 5000, static_cast<Timestamp>(rng() % 400)};
            if (oracle.count(id)) expected = QueueError::duplicate_id;
            assert(a.insert(e) == expected);
            assert(b.insert(e) == expected);
            if (expected == QueueError::none) oracle[id] = e;
        } else if (op == 1) {
            PriorityKey key = static_cast<PriorityKey>(rng() % 10000) - 5000;
            if (!oracle.count(id)) expected = QueueError::missing_id;
            else if (key >= oracle[id].priority_key) expected = QueueError::key_not_decreased;
            assert(a.decreaseKey(id, key) == expected);
            assert(b.decreaseKey(id, key) == expected);
            if (expected == QueueError::none) oracle[id].priority_key = key;
        } else if (op == 2) {
            if (!oracle.count(id)) expected = QueueError::missing_id;
            assert(a.deletePatient(id) == expected);
            assert(b.deletePatient(id) == expected);
            if (expected == QueueError::none) oracle.erase(id);
        } else if (op == 3) {
            ExtractResult x = a.extractMin();
            ExtractResult y = b.extractMin();
            assert(x.ok == y.ok && x.error == y.error);
            if (oracle.empty()) assert(!x.ok && x.error == QueueError::empty_queue);
            else {
                std::map<PatientId, QueueEntry>::iterator best = oracle.begin();
                for (std::map<PatientId, QueueEntry>::iterator it = oracle.begin(); it != oracle.end(); ++it)
                    if (entryLess(it->second, best->second)) best = it;
                assert(x.ok && x.entry.patient_id == best->first && y.entry.patient_id == best->first);
                assert(x.entry.priority_key == best->second.priority_key);
                assert(y.entry.arrival_time == best->second.arrival_time);
                oracle.erase(best);
            }
        } else {
            std::vector<QueueEntry> entries;
            int n = static_cast<int>(rng() % 32);
            for (int i = 0; i < n; ++i) entries.push_back(QueueEntry{static_cast<PatientId>(rng() % 50), static_cast<PriorityKey>(rng() % 500), i});
            bool duplicate = false;
            std::map<PatientId, QueueEntry> new_map;
            for (std::size_t i = 0; i < entries.size(); ++i) {
                if (new_map.count(entries[i].patient_id)) duplicate = true;
                new_map[entries[i].patient_id] = entries[i];
            }
            expected = duplicate ? QueueError::duplicate_id : QueueError::none;
            assert(a.buildHeap(entries) == expected);
            assert(b.buildHeap(entries) == expected);
            if (!duplicate) oracle.swap(new_map);
        }
        assert(a.size() == oracle.size() && b.size() == oracle.size());
        assert(a.validate() && b.validate());
        assert(b.physicalSize() <= 2 * b.size());
    }
    std::vector<QueueEntry> ties;
    ties.push_back(QueueEntry{30, 5, 100});
    ties.push_back(QueueEntry{20, 5, 90});
    ties.push_back(QueueEntry{10, 5, 90});
    assert(a.buildHeap(ties) == QueueError::none);
    assert(b.buildHeap(ties) == QueueError::none);
    for (int id = 10; id <= 30; id += 10) {
        assert(a.extractMin().entry.patient_id == static_cast<PatientId>(id));
        assert(b.extractMin().entry.patient_id == static_cast<PatientId>(id));
    }
    assert(a.empty() && b.empty());
    std::cout << "1000000 differential operations passed; tie-breaking passed\n";
}
