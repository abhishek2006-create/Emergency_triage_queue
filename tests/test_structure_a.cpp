
#include "src/structA/indexed_heap.hpp"

#include <iostream>
#include <cassert>
#include <vector>

using namespace std;
using namespace triage;

int main() {
    IndexedBinaryMinHeap heap;

    assert(heap.empty());

    assert(!heap.insert({101, 20, 100}).has_value());
    assert(!heap.insert({102, 10, 110}).has_value());
    assert(!heap.insert({103, 30, 120}).has_value());
    assert(!heap.insert({104, 5, 130}).has_value());

    assert(heap.size() == 4);
    assert(heap.validate());

    assert(heap.insert({101, 1, 100}) ==
           QueueError::duplicate_id);

    assert(!heap.decreaseKey(103, 2).has_value());
    assert(heap.validate());

    auto first = heap.extractMin();

    assert(first.ok());
    assert(first.value->patient_id == 103);
    assert(heap.validate());

    assert(!heap.deletePatient(102).has_value());
    assert(heap.validate());

    assert(heap.decreaseKey(101, 25) ==
           QueueError::key_not_decreased);

    assert(heap.deletePatient(999) ==
           QueueError::missing_id);

    vector<QueueEntry> patients = {
        {201, 40, 100},
        {202, 15, 110},
        {203, 25, 120},
        {204, 5, 130},
        {205, 10, 140}
    };

    assert(!heap.buildHeap(patients).has_value());
    assert(heap.size() == 5);
    assert(heap.validate());

    vector<QueueEntry> duplicates = {
        {301, 1, 100},
        {301, 2, 110}
    };

    assert(heap.buildHeap(duplicates) ==
           QueueError::duplicate_id);

    assert(heap.size() == 5);
    assert(heap.validate());

    vector<PatientId> expected = {
        204, 205, 202, 203, 201
    };

    for (PatientId id : expected) {
        auto result = heap.extractMin();

        assert(result.ok());
        assert(result.value->patient_id == id);
        assert(heap.validate());
    }

    assert(heap.empty());

    assert(heap.extractMin().error ==
           QueueError::empty_queue);

    vector<QueueEntry> ties = {
        {30, 5, 100},
        {20, 5, 90},
        {10, 5, 90}
    };

    assert(!heap.buildHeap(ties).has_value());

    assert(heap.extractMin().value->patient_id == 10);
    assert(heap.extractMin().value->patient_id == 20);
    assert(heap.extractMin().value->patient_id == 30);

    assert(heap.empty());

    vector<QueueEntry> many;

    for (int i = 1; i <= 1000; i++) {
        many.push_back({
            static_cast<PatientId>(i),
            static_cast<PriorityKey>(1001 - i),
            i
        });
    }

    assert(!heap.buildHeap(many).has_value());
    assert(heap.validate());

    for (int i = 1; i <= 500; i++) {
        assert(!heap.deletePatient(i).has_value());
        assert(heap.validate());
    }

    for (int i = 0; i < 500; i++) {
        auto result = heap.extractMin();
        assert(result.ok());
        assert(heap.validate());
    }

    assert(heap.empty());

    cout << "All Structure A tests passed!" << endl;

    return 0;
}
