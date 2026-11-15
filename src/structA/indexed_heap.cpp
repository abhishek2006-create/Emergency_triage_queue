
#include "indexed_heap.hpp"

#include <algorithm>
#include <utility>

namespace triage {

bool IndexedBinaryMinHeap::lessEntry(
    const QueueEntry& a,
    const QueueEntry& b
) {
    if (a.priority_key != b.priority_key)
        return a.priority_key < b.priority_key;

    if (a.arrival_time != b.arrival_time)
        return a.arrival_time < b.arrival_time;

    return a.patient_id < b.patient_id;
}

void IndexedBinaryMinHeap::swapEntries(
    std::size_t i,
    std::size_t j
) {
    if (i == j)
        return;

    std::swap(heap_[i], heap_[j]);

    locator_.insert(heap_[i].patient_id, i);
    locator_.insert(heap_[j].patient_id, j);
}

void IndexedBinaryMinHeap::siftUp(
    std::size_t index
) {
    while (index > 0) {
        std::size_t parent = (index - 1) / 2;

        if (!lessEntry(heap_[index], heap_[parent]))
            break;

        swapEntries(index, parent);
        index = parent;
    }
}

void IndexedBinaryMinHeap::siftDown(
    std::size_t index
) {
    std::size_t n = heap_.size();

    while (true) {
        std::size_t smallest = index;
        std::size_t left = 2 * index + 1;
        std::size_t right = 2 * index + 2;

        if (left < n &&
            lessEntry(heap_[left], heap_[smallest])) {
            smallest = left;
        }

        if (right < n &&
            lessEntry(heap_[right], heap_[smallest])) {
            smallest = right;
        }

        if (smallest == index)
            break;

        swapEntries(index, smallest);
        index = smallest;
    }
}

std::optional<QueueError>
IndexedBinaryMinHeap::insert(
    const QueueEntry& entry
) {
    if (locator_.contains(entry.patient_id))
        return QueueError::duplicate_id;

    std::size_t index = heap_.size();

    heap_.push_back(entry);
    locator_.insert(entry.patient_id, index);

    siftUp(index);

    return std::nullopt;
}

std::optional<QueueError>
IndexedBinaryMinHeap::decreaseKey(
    PatientId id,
    PriorityKey new_key
) {
    if (!locator_.contains(id))
        return QueueError::missing_id;

    std::size_t index = locator_.find(id);

    if (new_key >= heap_[index].priority_key)
        return QueueError::key_not_decreased;

    heap_[index].priority_key = new_key;

    siftUp(index);

    return std::nullopt;
}

void IndexedBinaryMinHeap::removeAt(
    std::size_t index
) {
    std::size_t last = heap_.size() - 1;
    PatientId removedId = heap_[index].patient_id;

    swapEntries(index, last);

    locator_.erase(removedId);
    heap_.pop_back();

    if (index >= heap_.size())
        return;

    if (index > 0 &&
        lessEntry(heap_[index],
                  heap_[(index - 1) / 2])) {
        siftUp(index);
    } else {
        siftDown(index);
    }
}

Result<QueueEntry>
IndexedBinaryMinHeap::extractMin() {
    if (heap_.empty()) {
        return {std::nullopt, QueueError::empty_queue};
    }

    QueueEntry result = heap_[0];

    removeAt(0);

    return {result, std::nullopt};
}

std::optional<QueueError>
IndexedBinaryMinHeap::deletePatient(
    PatientId id
) {
    if (!locator_.contains(id))
        return QueueError::missing_id;

    std::size_t index = locator_.find(id);

    removeAt(index);

    return std::nullopt;
}

std::optional<QueueError>
IndexedBinaryMinHeap::buildHeap(
    const std::vector<QueueEntry>& entries
) {
    IndexedBinaryMinHeap temp;

    temp.heap_ = entries;

    for (std::size_t i = 0;
         i < temp.heap_.size(); i++) {

        PatientId id = temp.heap_[i].patient_id;

        if (temp.locator_.contains(id))
            return QueueError::duplicate_id;

        temp.locator_.insert(id, i);
    }

    for (std::size_t i = temp.heap_.size() / 2;
         i > 0; i--) {
        temp.siftDown(i - 1);
    }

    heap_.swap(temp.heap_);
    std::swap(locator_, temp.locator_);

    return std::nullopt;
}

bool IndexedBinaryMinHeap::empty() const {
    return heap_.empty();
}

std::size_t IndexedBinaryMinHeap::size() const {
    return heap_.size();
}

bool IndexedBinaryMinHeap::validate() const {
    if (locator_.size() != heap_.size())
        return false;

    for (std::size_t i = 0;
         i < heap_.size(); i++) {

        PatientId id = heap_[i].patient_id;

        if (!locator_.contains(id))
            return false;

        if (locator_.find(id) != i)
            return false;

        if (i > 0) {
            std::size_t parent = (i - 1) / 2;

            if (lessEntry(heap_[i], heap_[parent]))
                return false;
        }
    }

    return true;
}

}
