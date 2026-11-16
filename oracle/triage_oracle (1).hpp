#include "triage_oracle.hpp"
#include <algorithm>
#include <limits>

namespace triage {

std::size_t TriageOracle::findIndex(PatientId id) const {
    for (std::size_t i = 0; i < patients_.size(); ++i)
        if (patients_[i].patient_id == id) return i;
    return patients_.size();
}

bool TriageOracle::lessEntry(const QueueEntry& a, const QueueEntry& b) {
    if (a.priority_key != b.priority_key) return a.priority_key < b.priority_key;
    if (a.arrival_time != b.arrival_time) return a.arrival_time < b.arrival_time;
    return a.patient_id < b.patient_id;
}

QueueError TriageOracle::insert(const QueueEntry& entry) {
    if (findIndex(entry.patient_id) != patients_.size()) return QueueError::duplicate_id;
    patients_.push_back(entry);
    return QueueError::none;
}

ExtractResult TriageOracle::extractMin() {
    if (patients_.empty()) return ExtractResult{false, QueueEntry{}, QueueError::empty_queue};
    std::size_t best = 0;
    for (std::size_t i = 1; i < patients_.size(); ++i)
        if (lessEntry(patients_[i], patients_[best])) best = i;
    QueueEntry result = patients_[best];
    patients_[best] = patients_.back();
    patients_.pop_back();
    return ExtractResult{true, result, QueueError::none};
}

QueueError TriageOracle::decreaseKey(PatientId id, PriorityKey new_key) {
    std::size_t index = findIndex(id);
    if (index == patients_.size()) return QueueError::missing_id;
    if (new_key >= patients_[index].priority_key) return QueueError::key_not_decreased;
    patients_[index].priority_key = new_key;
    return QueueError::none;
}

QueueError TriageOracle::deletePatient(PatientId id) {
    std::size_t index = findIndex(id);
    if (index == patients_.size()) return QueueError::missing_id;
    patients_[index] = patients_.back();
    patients_.pop_back();
    return QueueError::none;
}

QueueError TriageOracle::buildHeap(const std::vector<QueueEntry>& entries) {
    std::vector<PatientId> ids;
    ids.reserve(entries.size());
    for (std::size_t i = 0; i < entries.size(); ++i) ids.push_back(entries[i].patient_id);
    std::sort(ids.begin(), ids.end());
    for (std::size_t i = 1; i < ids.size(); ++i)
        if (ids[i] == ids[i - 1]) return QueueError::duplicate_id;
    std::vector<QueueEntry> replacement(entries);
    patients_.swap(replacement);
    return QueueError::none;
}

bool TriageOracle::empty() const { return patients_.empty(); }
std::size_t TriageOracle::size() const { return patients_.size(); }
const std::vector<QueueEntry>& TriageOracle::entries() const { return patients_; }

bool TriageOracle::validate() const {
    std::vector<PatientId> ids;
    ids.reserve(patients_.size());
    for (std::size_t i = 0; i < patients_.size(); ++i) ids.push_back(patients_[i].patient_id);
    std::sort(ids.begin(), ids.end());
    for (std::size_t i = 1; i < ids.size(); ++i)
        if (ids[i] == ids[i - 1]) return false;
    return true;
}

}
