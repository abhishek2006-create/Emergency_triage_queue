#ifndef TRIAGE_ORACLE_HPP
#define TRIAGE_ORACLE_HPP

#include "../src/common/triage_types.hpp"
#include <cstddef>
#include <vector>

namespace triage {

class TriageOracle {
private:
    std::vector<QueueEntry> patients_;

    std::size_t findIndex(PatientId id) const;
    static bool lessEntry(const QueueEntry& a, const QueueEntry& b);

public:
    QueueError insert(const QueueEntry& entry);
    ExtractResult extractMin();
    QueueError decreaseKey(PatientId id, PriorityKey new_key);
    QueueError deletePatient(PatientId id);
    QueueError buildHeap(const std::vector<QueueEntry>& entries);

    bool empty() const;
    std::size_t size() const;
    const std::vector<QueueEntry>& entries() const;
    bool validate() const;
};

}

#endif
