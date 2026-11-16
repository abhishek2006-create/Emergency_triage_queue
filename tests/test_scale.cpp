#include "../src/structA/indexed_heap.hpp"
#include "../src/structB/lazy_heap.hpp"
#include <cassert>
#include <iostream>
#include <vector>
using namespace triage;
int main() {
    std::vector<QueueEntry> entries;
    entries.reserve(1000000);
    for (std::uint64_t i = 0; i < 1000000; ++i)
        entries.push_back(QueueEntry{i + 1, static_cast<PriorityKey>(1000000 - i), static_cast<Timestamp>(i)});
    IndexedBinaryMinHeap a;
    LazyBinaryMinHeap b;
    assert(a.buildHeap(entries) == QueueError::none);
    assert(b.buildHeap(entries) == QueueError::none);
    assert(a.size() == 1000000 && b.size() == 1000000);
    assert(a.validate() && b.validate());
    for (std::uint64_t i = 0; i < 1000000; ++i) {
        ExtractResult x = a.extractMin();
        ExtractResult y = b.extractMin();
        assert(x.ok && y.ok);
        assert(x.entry.patient_id == 1000000 - i);
        assert(y.entry.patient_id == x.entry.patient_id);
    }
    assert(a.empty() && b.empty());
    assert(a.validate() && b.validate());
    std::cout << "1000000-element build/extraction test passed\n";
}
