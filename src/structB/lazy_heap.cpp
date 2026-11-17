#include "lazy_heap.hpp"
#include <utility>
#include <limits>
#include <stdexcept>
namespace triage {
bool LazyBinaryMinHeap::lessNode(const Node& a, const Node& b) {
    if (entryLess(a.entry, b.entry)) return true;
    if (entryLess(b.entry, a.entry)) return false;
    return a.version < b.version;
}
bool LazyBinaryMinHeap::valid(const Node& node) const {
    std::unordered_map<PatientId, State>::const_iterator it = active_.find(node.entry.patient_id);
    return it != active_.end() && it->second.version == node.version;
}
std::uint64_t LazyBinaryMinHeap::freshVersion() {
    if (next_version_ == 0) throw std::overflow_error("Version counter exhausted");
    return next_version_++;
}
void LazyBinaryMinHeap::siftUp(std::size_t i) {
    while (i > 0) {
        std::size_t p = (i - 1) / 2;
        if (!lessNode(heap_[i], heap_[p])) break;
        std::swap(heap_[i], heap_[p]);
        i = p;
    }
}
void LazyBinaryMinHeap::siftDown(std::size_t i) {
    const std::size_t n = heap_.size();
    while (i < n) {
        std::size_t best = i;
        if (i <= (n - 1) / 2) {
            std::size_t l = 2 * i + 1;
            std::size_t r = l + 1;
            if (l < n && lessNode(heap_[l], heap_[best])) best = l;
            if (r < n && lessNode(heap_[r], heap_[best])) best = r;
        }
        if (best == i) break;
        std::swap(heap_[i], heap_[best]);
        i = best;
    }
}
void LazyBinaryMinHeap::pushNode(const Node& node) {
    heap_.push_back(node);
    siftUp(heap_.size() - 1);
}
LazyBinaryMinHeap::Node LazyBinaryMinHeap::popNode() {
    Node answer = heap_.front();
    heap_.front() = heap_.back();
    heap_.pop_back();
    if (!heap_.empty()) siftDown(0);
    return answer;
}
void LazyBinaryMinHeap::compact() {
    std::vector<Node> fresh;
    fresh.reserve(active_.size());
    for (std::size_t i = 0; i < heap_.size(); ++i) if (valid(heap_[i])) fresh.push_back(heap_[i]);
    heap_.swap(fresh);
    for (std::size_t i = heap_.size() / 2; i > 0; --i) siftDown(i - 1);
}
void LazyBinaryMinHeap::compactIfNeeded() {
    if (staleCount() > active_.size()) compact();
}
QueueError LazyBinaryMinHeap::insert(const QueueEntry& entry) {
    if (active_.count(entry.patient_id)) return QueueError::duplicate_id;
    std::uint64_t v = freshVersion();
    active_[entry.patient_id] = State{entry, v};
    pushNode(Node{entry, v});
    compactIfNeeded();
    return QueueError::none;
}
QueueError LazyBinaryMinHeap::decreaseKey(PatientId id, PriorityKey new_key) {
    std::unordered_map<PatientId, State>::iterator it = active_.find(id);
    if (it == active_.end()) return QueueError::missing_id;
    if (new_key >= it->second.entry.priority_key) return QueueError::key_not_decreased;
    QueueEntry revised = it->second.entry;
    revised.priority_key = new_key;
    std::uint64_t v = freshVersion();
    it->second = State{revised, v};
    pushNode(Node{revised, v});
    compactIfNeeded();
    return QueueError::none;
}
QueueError LazyBinaryMinHeap::deletePatient(PatientId id) {
    std::unordered_map<PatientId, State>::iterator it = active_.find(id);
    if (it == active_.end()) return QueueError::missing_id;
    active_.erase(it);
    QueueEntry marker = {id, std::numeric_limits<PriorityKey>::max(), 0};
    pushNode(Node{marker, 0});
    compactIfNeeded();
    return QueueError::none;
}
ExtractResult LazyBinaryMinHeap::extractMin() {
    while (!heap_.empty() && !valid(heap_.front())) popNode();
    if (heap_.empty()) return ExtractResult{false, QueueEntry{}, QueueError::empty_queue};
    Node best = popNode();
    active_.erase(best.entry.patient_id);
    compactIfNeeded();
    return ExtractResult{true, best.entry, QueueError::none};
}
QueueError LazyBinaryMinHeap::buildHeap(const std::vector<QueueEntry>& entries) {
    LazyBinaryMinHeap replacement;
    replacement.heap_.reserve(entries.size());
    replacement.active_.reserve(entries.size());
    for (std::size_t i = 0; i < entries.size(); ++i) {
        PatientId id = entries[i].patient_id;
        if (replacement.active_.count(id)) return QueueError::duplicate_id;
        std::uint64_t v = replacement.freshVersion();
        replacement.active_[id] = State{entries[i], v};
        replacement.heap_.push_back(Node{entries[i], v});
    }
    for (std::size_t i = replacement.heap_.size() / 2; i > 0; --i) replacement.siftDown(i - 1);
    heap_.swap(replacement.heap_);
    active_.swap(replacement.active_);
    std::swap(next_version_, replacement.next_version_);
    return QueueError::none;
}
bool LazyBinaryMinHeap::empty() const { return active_.empty(); }
std::size_t LazyBinaryMinHeap::size() const { return active_.size(); }
std::size_t LazyBinaryMinHeap::physicalSize() const { return heap_.size(); }
std::size_t LazyBinaryMinHeap::staleCount() const { return heap_.size() - active_.size(); }
bool LazyBinaryMinHeap::validate() const {
    if (heap_.size() < active_.size() || staleCount() > active_.size()) return false;
    std::unordered_map<PatientId, std::size_t> counts;
    for (std::size_t i = 0; i < heap_.size(); ++i) {
        if (i && lessNode(heap_[i], heap_[(i - 1) / 2])) return false;
        if (valid(heap_[i])) ++counts[heap_[i].entry.patient_id];
    }
    if (counts.size() != active_.size()) return false;
    for (std::unordered_map<PatientId, State>::const_iterator it = active_.begin(); it != active_.end(); ++it) {
        if (counts[it->first] != 1) return false;
    }
    return true;
}
}
