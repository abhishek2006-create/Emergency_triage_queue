
#ifndef INDEXED_HEAP_HPP
#define INDEXED_HEAP_HPP

#include "locator_table.hpp"

#include <vector>
#include <cstdint>
#include <cstddef>
#include <optional>

namespace triage {

using PatientId = std::uint64_t;
using PriorityKey = std::int64_t;
using Timestamp = std::int64_t;

struct QueueEntry {
    PatientId patient_id;
    PriorityKey priority_key;
    Timestamp arrival_time;
};

enum class QueueError {
    duplicate_id,
    missing_id,
    empty_queue,
    key_not_decreased
};

template <typename T>
struct Result {
    std::optional<T> value;
    std::optional<QueueError> error;

    bool ok() const {
        return value.has_value() && !error.has_value();
    }
};

class IndexedBinaryMinHeap {
private:
    std::vector<QueueEntry> heap_;
    LocatorTable locator_;

    static bool lessEntry(
        const QueueEntry& a,
        const QueueEntry& b
    );

    void swapEntries(std::size_t i, std::size_t j);
    void siftUp(std::size_t index);
    void siftDown(std::size_t index);
    void removeAt(std::size_t index);

public:
    IndexedBinaryMinHeap() = default;

    std::optional<QueueError> insert(
        const QueueEntry& entry
    );

    Result<QueueEntry> extractMin();

    std::optional<QueueError> decreaseKey(
        PatientId id,
        PriorityKey new_key
    );

    std::optional<QueueError> deletePatient(
        PatientId id
    );

    std::optional<QueueError> buildHeap(
        const std::vector<QueueEntry>& entries
    );

    bool empty() const;
    std::size_t size() const;
    bool validate() const;
};

}

#endif
