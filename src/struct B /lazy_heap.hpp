#ifndef LAZY_HEAP_HPP
#define LAZY_HEAP_HPP
#include "../common/triage_types.hpp"
#include <vector>
#include <unordered_map>
namespace triage {
class LazyBinaryMinHeap {
    struct Node { QueueEntry entry; std::uint64_t version; };
    struct State { QueueEntry entry; std::uint64_t version; };
    std::vector<Node> heap_;
    std::unordered_map<PatientId, State> active_;
    std::uint64_t next_version_ = 1;
    static bool lessNode(const Node& a, const Node& b);
    bool valid(const Node& node) const;
    std::uint64_t freshVersion();
    void siftUp(std::size_t i);
    void siftDown(std::size_t i);
    void pushNode(const Node& node);
    Node popNode();
    void compact();
    void compactIfNeeded();
public:
    QueueError insert(const QueueEntry& entry);
    ExtractResult extractMin();
    QueueError decreaseKey(PatientId id, PriorityKey new_key);
    QueueError deletePatient(PatientId id);
    QueueError buildHeap(const std::vector<QueueEntry>& entries);
    bool empty() const;
    std::size_t size() const;
    std::size_t physicalSize() const;
    std::size_t staleCount() const;
    bool validate() const;
};
}
#endif
