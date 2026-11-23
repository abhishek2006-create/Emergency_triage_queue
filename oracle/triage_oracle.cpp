#include "triage_oracle.hpp"
#include <cassert>
#include <iostream>
#include <limits>
#include <vector>

using namespace triage;

int main() {
    TriageOracle q;
    assert(q.empty());
    assert(q.extractMin().error == QueueError::empty_queue);
    assert(q.deletePatient(10) == QueueError::missing_id);
    assert(q.decreaseKey(10, 0) == QueueError::missing_id);
    assert(q.insert(QueueEntry{3, 5, 50}) == QueueError::none);
    assert(q.insert(QueueEntry{2, 5, 40}) == QueueError::none);
    assert(q.insert(QueueEntry{1, 5, 40}) == QueueError::none);
    assert(q.insert(QueueEntry{3, 1, 0}) == QueueError::duplicate_id);
    assert(q.size() == 3 && q.validate());
    assert(q.extractMin().entry.patient_id == 1);
    assert(q.extractMin().entry.patient_id == 2);
    assert(q.decreaseKey(3, 6) == QueueError::key_not_decreased);
    assert(q.decreaseKey(3, -6) == QueueError::none);
    assert(q.extractMin().entry.patient_id == 3);
    assert(q.empty());
    assert(q.buildHeap(std::vector<QueueEntry>{QueueEntry{9, 9, 1}, QueueEntry{8, 8, 2}}) == QueueError::none);
    assert(q.buildHeap(std::vector<QueueEntry>{QueueEntry{7, 7, 1}, QueueEntry{7, 1, 2}}) == QueueError::duplicate_id);
    assert(q.size() == 2);
    assert(q.extractMin().entry.patient_id == 8);
    assert(q.deletePatient(9) == QueueError::none);
    assert(q.empty());
    assert(q.insert(QueueEntry{99, std::numeric_limits<PriorityKey>::min(), 0}) == QueueError::none);
    assert(q.extractMin().entry.patient_id == 99);
    assert(q.buildHeap(std::vector<QueueEntry>()) == QueueError::none);
    assert(q.empty() && q.validate());
    std::cout << "PASS oracle unit tests\n";
}
