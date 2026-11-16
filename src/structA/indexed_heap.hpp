#ifndef INDEXED_HEAP_HPP
#define INDEXED_HEAP_HPP
#include "../common/triage_types.hpp"
#include "locator_table.hpp"
#include <vector>
namespace triage {
class IndexedBinaryMinHeap {
    std::vector<QueueEntry> heap_;
    LocatorTable locator_;
    void swapEntries(std::size_t a, std::size_t b);
    void siftUp(std::size_t i);
    void siftDown(std::size_t i);
    void removeAt(std::size_t i);
public:
    QueueError insert(const QueueEntry& entry);
    ExtractResult extractMin();
    QueueError decreaseKey(PatientId id, PriorityKey new_key);
    QueueError deletePatient(PatientId id);
    QueueError buildHeap(const std::vector<QueueEntry>& entries);
    bool empty() const;
    std::size_t size() const;
    bool validate() const;
};
}
#endif
