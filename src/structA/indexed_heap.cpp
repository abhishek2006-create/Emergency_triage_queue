#include "indexed_heap.hpp"
#include <utility>
namespace triage {
void IndexedBinaryMinHeap::swapEntries(std::size_t a, std::size_t b) {
    if (a == b) return;
    std::swap(heap_[a], heap_[b]);
    locator_.insert(heap_[a].patient_id, a);
    locator_.insert(heap_[b].patient_id, b);
}
void IndexedBinaryMinHeap::siftUp(std::size_t i) {
    while (i > 0) {
        std::size_t p = (i - 1) / 2;
        if (!entryLess(heap_[i], heap_[p])) break;
        swapEntries(i, p);
        i = p;
    }
}
void IndexedBinaryMinHeap::siftDown(std::size_t i) {
    const std::size_t n = heap_.size();
    while (i < n) {
        std::size_t best = i;
        if (i <= (n - 1) / 2) {
            std::size_t l = 2 * i + 1;
            std::size_t r = l + 1;
            if (l < n && entryLess(heap_[l], heap_[best])) best = l;
            if (r < n && entryLess(heap_[r], heap_[best])) best = r;
        }
        if (best == i) break;
        swapEntries(i, best);
        i = best;
    }
}
QueueError IndexedBinaryMinHeap::insert(const QueueEntry& entry) {
    if (locator_.contains(entry.patient_id)) return QueueError::duplicate_id;
    std::size_t index = heap_.size();
    heap_.push_back(entry);
    locator_.insert(entry.patient_id, index);
    siftUp(index);
    return QueueError::none;
}
void IndexedBinaryMinHeap::removeAt(std::size_t i) {
    std::size_t last = heap_.size() - 1;
    PatientId removed = heap_[i].patient_id;
    swapEntries(i, last);
    locator_.erase(removed);
    heap_.pop_back();
    if (i >= heap_.size()) return;
    if (i > 0 && entryLess(heap_[i], heap_[(i - 1) / 2])) siftUp(i);
    else siftDown(i);
}
ExtractResult IndexedBinaryMinHeap::extractMin() {
    if (heap_.empty()) return ExtractResult{false, QueueEntry{}, QueueError::empty_queue};
    QueueEntry answer = heap_[0];
    removeAt(0);
    return ExtractResult{true, answer, QueueError::none};
}
QueueError IndexedBinaryMinHeap::decreaseKey(PatientId id, PriorityKey new_key) {
    if (!locator_.contains(id)) return QueueError::missing_id;
    std::size_t i = locator_.find(id);
    if (new_key >= heap_[i].priority_key) return QueueError::key_not_decreased;
    heap_[i].priority_key = new_key;
    siftUp(i);
    return QueueError::none;
}
QueueError IndexedBinaryMinHeap::deletePatient(PatientId id) {
    if (!locator_.contains(id)) return QueueError::missing_id;
    removeAt(locator_.find(id));
    return QueueError::none;
}
QueueError IndexedBinaryMinHeap::buildHeap(const std::vector<QueueEntry>& entries) {
    IndexedBinaryMinHeap candidate;
    candidate.heap_ = entries;
    for (std::size_t i = 0; i < entries.size(); ++i) {
        if (candidate.locator_.contains(entries[i].patient_id)) return QueueError::duplicate_id;
        candidate.locator_.insert(entries[i].patient_id, i);
    }
    for (std::size_t i = entries.size() / 2; i > 0; --i) candidate.siftDown(i - 1);
    heap_.swap(candidate.heap_);
    std::swap(locator_, candidate.locator_);
    return QueueError::none;
}
bool IndexedBinaryMinHeap::empty() const { return heap_.empty(); }
std::size_t IndexedBinaryMinHeap::size() const { return heap_.size(); }
bool IndexedBinaryMinHeap::validate() const {
    if (locator_.size() != heap_.size()) return false;
    for (std::size_t i = 0; i < heap_.size(); ++i) {
        if (!locator_.contains(heap_[i].patient_id) || locator_.find(heap_[i].patient_id) != i) return false;
        if (i && entryLess(heap_[i], heap_[(i - 1) / 2])) return false;
    }
    return true;
}
}
