#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>

#if defined(__has_include)
#  if __has_include(<optional>)
#    include <optional>
#  else
#    include <type_traits>
#    include <utility>

namespace std {
struct nullopt_t {
  explicit constexpr nullopt_t(int) {}
  constexpr nullopt_t() = default;
};
constexpr nullopt_t nullopt{0};

template <typename T>
class optional {
 public:
  optional() : has_value_(false), value_() {}
  optional(nullopt_t) : has_value_(false), value_() {}
  optional(const T& value) : has_value_(true), value_(value) {}
  optional(T&& value) noexcept(std::is_nothrow_move_constructible<T>::value)
      : has_value_(true), value_(std::move(value)) {}
  optional(const optional&) = default;
  optional(optional&&) noexcept(std::is_nothrow_move_constructible<T>::value) = default;
  optional& operator=(const optional&) = default;
  optional& operator=(optional&&) noexcept(std::is_nothrow_move_assignable<T>::value) = default;
  optional& operator=(const T& value) {
    value_ = value;
    has_value_ = true;
    return *this;
  }
  optional& operator=(T&& value) {
    value_ = std::move(value);
    has_value_ = true;
    return *this;
  }

  constexpr bool has_value() const noexcept { return has_value_; }
  constexpr explicit operator bool() const noexcept { return has_value_; }

  constexpr T& value() & { return value_; }
  constexpr const T& value() const & { return value_; }
  constexpr T&& value() && { return std::move(value_); }
  constexpr const T&& value() const && { return std::move(value_); }

  constexpr T& operator*() & { return value_; }
  constexpr const T& operator*() const & { return value_; }
  constexpr T&& operator*() && { return std::move(value_); }
  constexpr const T&& operator*() const && { return std::move(value_); }

  constexpr T* operator->() { return &value_; }
  constexpr const T* operator->() const { return &value_; }

  void reset() noexcept(std::is_nothrow_destructible<T>::value) { has_value_ = false; }

 private:
  bool has_value_;
  T value_;
};

template <typename T>
inline bool operator==(const optional<T>& lhs, const optional<T>& rhs) {
  if (lhs.has_value() != rhs.has_value()) {
    return false;
  }
  if (!lhs.has_value()) {
    return true;
  }
  return *lhs == *rhs;
}

template <typename T>
inline bool operator==(const optional<T>& lhs, nullopt_t) {
  return !lhs.has_value();
}

template <typename T>
inline bool operator==(nullopt_t, const optional<T>& rhs) {
  return !rhs.has_value();
}

template <typename T, typename U>
inline bool operator==(const optional<T>& lhs, const U& rhs) {
  return lhs.has_value() && *lhs == rhs;
}

template <typename T, typename U>
inline bool operator==(const U& lhs, const optional<T>& rhs) {
  return rhs.has_value() && lhs == *rhs;
}
}  // namespace std
#  endif
#else
#  include <optional>
#endif

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace triage {

using PatientId = std::uint64_t;
using PriorityKey = std::int64_t;
using Timestamp = std::int64_t;  // simulated minutes since the chosen epoch

struct QueueEntry {
  PatientId patient_id;
  PriorityKey priority_key;  // lower value has higher treatment priority
  Timestamp arrival_time;
};

enum class QueueError { duplicate_id, missing_id, empty_queue, key_not_decreased };

template <typename T>
struct Result {
  std::optional<T> value;
  std::optional<QueueError> error;
  [[nodiscard]] bool ok() const { return value.has_value(); }
};

class ITriageQueue {
 public:
  virtual ~ITriageQueue() = default;
  virtual std::optional<QueueError> insert(const QueueEntry& entry) = 0;
  virtual Result<QueueEntry> extractMin() = 0;
  virtual std::optional<QueueError> decreaseKey(PatientId id, PriorityKey new_key) = 0;
  virtual std::optional<QueueError> deletePatient(PatientId id) = 0;
  virtual std::optional<QueueError> buildHeap(const std::vector<QueueEntry>& entries) = 0;
  virtual bool empty() const = 0;
  virtual std::size_t size() const = 0;
};

inline bool operator<(const QueueEntry& lhs, const QueueEntry& rhs) {
  if (lhs.priority_key != rhs.priority_key) {
    return lhs.priority_key < rhs.priority_key;
  }
  if (lhs.arrival_time != rhs.arrival_time) {
    return lhs.arrival_time < rhs.arrival_time;
  }
  return lhs.patient_id < rhs.patient_id;
}

inline bool isLessByPriority(const QueueEntry& lhs, const QueueEntry& rhs) {
  return lhs < rhs;
}

class IndexedBinaryMinHeap final : public ITriageQueue {
 public:
  std::optional<QueueError> insert(const QueueEntry& entry) override {
    if (entry.patient_id == 0) {
      return QueueError::duplicate_id;
    }
    if (locator_.find(entry.patient_id) != locator_.end()) {
      return QueueError::duplicate_id;
    }

    heap_.push_back(entry);
    locator_[entry.patient_id] = heap_.size() - 1;
    siftUp(heap_.size() - 1);
    return std::nullopt;
  }

  Result<QueueEntry> extractMin() override {
    Result<QueueEntry> result{};
    if (heap_.empty()) {
      result.error = QueueError::empty_queue;
      return result;
    }

    QueueEntry minimum = heap_.front();
    const auto last_index = heap_.size() - 1;
    if (last_index != 0) {
      heap_[0] = heap_[last_index];
      locator_[heap_[0].patient_id] = 0;
      locator_.erase(minimum.patient_id);
      heap_.pop_back();
      siftDown(0);
    } else {
      locator_.erase(minimum.patient_id);
      heap_.clear();
    }

    result.value = minimum;
    return result;
  }

  std::optional<QueueError> decreaseKey(PatientId id, PriorityKey new_key) override {
    auto it = locator_.find(id);
    if (it == locator_.end()) {
      return QueueError::missing_id;
    }

    std::size_t index = it->second;
    if (new_key >= heap_[index].priority_key) {
      return QueueError::key_not_decreased;
    }

    heap_[index].priority_key = new_key;
    siftUp(index);
    return std::nullopt;
  }

  std::optional<QueueError> deletePatient(PatientId id) override {
    auto it = locator_.find(id);
    if (it == locator_.end()) {
      return QueueError::missing_id;
    }

    const std::size_t index = it->second;
    const std::size_t last_index = heap_.size() - 1;
    if (index != last_index) {
      std::swap(heap_[index], heap_[last_index]);
      locator_[heap_[index].patient_id] = index;
      locator_.erase(id);
      heap_.pop_back();
      if (index < heap_.size()) {
        siftDown(index);
        siftUp(index);
      }
    } else {
      locator_.erase(id);
      heap_.pop_back();
    }
    return std::nullopt;
  }

  std::optional<QueueError> buildHeap(const std::vector<QueueEntry>& entries) override {
    std::unordered_map<PatientId, bool> seen;
    seen.reserve(entries.size());
    for (const auto& entry : entries) {
      if (entry.patient_id == 0 || seen[entry.patient_id]) {
        return QueueError::duplicate_id;
      }
      seen[entry.patient_id] = true;
    }

    heap_.assign(entries.begin(), entries.end());
    locator_.clear();
    for (std::size_t index = heap_.size() / 2; index-- > 0;) {
      siftDown(index);
    }
    for (std::size_t i = 0; i < heap_.size(); ++i) {
      locator_[heap_[i].patient_id] = i;
    }
    return std::nullopt;
  }

  bool empty() const override { return heap_.empty(); }
  std::size_t size() const override { return heap_.size(); }

 private:
  void siftUp(std::size_t index) {
    while (index > 0) {
      const std::size_t parent = (index - 1) / 2;
      if (!isLessByPriority(heap_[index], heap_[parent])) {
        break;
      }
      std::swap(heap_[index], heap_[parent]);
      locator_[heap_[index].patient_id] = index;
      locator_[heap_[parent].patient_id] = parent;
      index = parent;
    }
  }

  void siftDown(std::size_t index) {
    const std::size_t size = heap_.size();
    while (true) {
      std::size_t smallest = index;
      const std::size_t left = 2 * index + 1;
      const std::size_t right = left + 1;

      if (left < size && isLessByPriority(heap_[left], heap_[smallest])) {
        smallest = left;
      }
      if (right < size && isLessByPriority(heap_[right], heap_[smallest])) {
        smallest = right;
      }
      if (smallest == index) {
        break;
      }

      std::swap(heap_[index], heap_[smallest]);
      locator_[heap_[index].patient_id] = index;
      locator_[heap_[smallest].patient_id] = smallest;
      index = smallest;
    }
  }

  std::vector<QueueEntry> heap_;
  std::unordered_map<PatientId, std::size_t> locator_;
};

class LazyInvalidationBinaryHeap final : public ITriageQueue {
 public:
  struct HeapRecord {
    PatientId patient_id;
    std::uint64_t version;
    PriorityKey priority_key;
    Timestamp arrival_time;
  };

  std::optional<QueueError> insert(const QueueEntry& entry) override {
    if (entry.patient_id == 0 || versions_.find(entry.patient_id) != versions_.end()) {
      return QueueError::duplicate_id;
    }

    const std::uint64_t version = nextVersion();
    versions_[entry.patient_id] = version;
    heap_.push_back({entry.patient_id, version, entry.priority_key, entry.arrival_time});
    live_count_++;
    siftUp(heap_.size() - 1);
    return std::nullopt;
  }

  Result<QueueEntry> extractMin() override {
    Result<QueueEntry> result{};
    if (live_count_ == 0) {
      result.error = QueueError::empty_queue;
      return result;
    }

    while (!heap_.empty() && !isCurrent(heap_.front())) {
      removeTop();
    }

    if (heap_.empty()) {
      result.error = QueueError::empty_queue;
      return result;
    }

    const HeapRecord record = heap_.front();
    removeTop();

    auto it = versions_.find(record.patient_id);
    if (it != versions_.end()) {
      versions_.erase(it);
    }
    --live_count_;

    result.value = QueueEntry{record.patient_id, record.priority_key, record.arrival_time};
    return result;
  }

  std::optional<QueueError> decreaseKey(PatientId id, PriorityKey new_key) override {
    auto it = versions_.find(id);
    if (it == versions_.end()) {
      return QueueError::missing_id;
    }

    const std::uint64_t old_version = it->second;
    if (new_key >= lookupCurrentPriority(id)) {
      return QueueError::key_not_decreased;
    }

    const std::uint64_t new_version = nextVersion();
    it->second = new_version;
    for (HeapRecord& record : heap_) {
      if (record.patient_id == id && record.version == old_version) {
        record.priority_key = new_key;
        record.version = new_version;
      }
    }
    heapify();
    return std::nullopt;
  }

  std::optional<QueueError> deletePatient(PatientId id) override {
    auto it = versions_.find(id);
    if (it == versions_.end()) {
      return QueueError::missing_id;
    }

    const std::uint64_t old_version = it->second;
    versions_.erase(it);
    for (auto it2 = heap_.begin(); it2 != heap_.end();) {
      if (it2->patient_id == id && it2->version == old_version) {
        it2 = heap_.erase(it2);
      } else {
        ++it2;
      }
    }
    --live_count_;
    heapify();
    return std::nullopt;
  }

  std::optional<QueueError> buildHeap(const std::vector<QueueEntry>& entries) override {
    std::unordered_map<PatientId, bool> seen;
    seen.reserve(entries.size());
    for (const auto& entry : entries) {
      if (entry.patient_id == 0 || seen[entry.patient_id]) {
        return QueueError::duplicate_id;
      }
      seen[entry.patient_id] = true;
    }

    heap_.clear();
    versions_.clear();
    live_count_ = 0;
    for (const auto& entry : entries) {
      const std::uint64_t version = nextVersion();
      versions_[entry.patient_id] = version;
      heap_.push_back({entry.patient_id, version, entry.priority_key, entry.arrival_time});
      ++live_count_;
    }
    heapify();
    return std::nullopt;
  }

  bool empty() const override { return live_count_ == 0; }
  std::size_t size() const override { return live_count_; }

 private:
  std::uint64_t nextVersion() {
    return ++next_version_;
  }

  bool isCurrent(const HeapRecord& record) const {
    auto it = versions_.find(record.patient_id);
    return it != versions_.end() && it->second == record.version;
  }

  PriorityKey lookupCurrentPriority(PatientId id) const {
    auto it = versions_.find(id);
    if (it == versions_.end()) {
      return 0;
    }
    for (const auto& record : heap_) {
      if (record.patient_id == id && record.version == it->second) {
        return record.priority_key;
      }
    }
    return 0;
  }

  static bool isLessRecord(const HeapRecord& lhs, const HeapRecord& rhs) {
    if (lhs.priority_key != rhs.priority_key) {
      return lhs.priority_key < rhs.priority_key;
    }
    if (lhs.arrival_time != rhs.arrival_time) {
      return lhs.arrival_time < rhs.arrival_time;
    }
    return lhs.patient_id < rhs.patient_id;
  }

  void heapify() {
    for (std::size_t index = heap_.size() / 2; index-- > 0;) {
      siftDown(index);
    }
  }

  void removeTop() {
    if (heap_.empty()) {
      return;
    }
    if (heap_.size() == 1) {
      heap_.pop_back();
      return;
    }
    heap_.front() = heap_.back();
    heap_.pop_back();
    siftDown(0);
  }

  void siftUp(std::size_t index) {
    while (index > 0) {
      const std::size_t parent = (index - 1) / 2;
      if (!isLessRecord(heap_[index], heap_[parent])) {
        break;
      }
      std::swap(heap_[index], heap_[parent]);
      index = parent;
    }
  }

  void siftDown(std::size_t index) {
    const std::size_t size = heap_.size();
    while (true) {
      std::size_t smallest = index;
      const std::size_t left = 2 * index + 1;
      const std::size_t right = left + 1;

      if (left < size && isLessRecord(heap_[left], heap_[smallest])) {
        smallest = left;
      }
      if (right < size && isLessRecord(heap_[right], heap_[smallest])) {
        smallest = right;
      }
      if (smallest == index) {
        break;
      }

      std::swap(heap_[index], heap_[smallest]);
      index = smallest;
    }
  }

  std::vector<HeapRecord> heap_;
  std::unordered_map<PatientId, std::uint64_t> versions_;
  std::uint64_t next_version_ = 0;
  std::size_t live_count_ = 0;
};

class UnsortedArrayOracle final : public ITriageQueue {
 public:
  std::optional<QueueError> insert(const QueueEntry& entry) override {
    if (entry.patient_id == 0) {
      return QueueError::duplicate_id;
    }
    for (const auto& existing : entries_) {
      if (existing.patient_id == entry.patient_id) {
        return QueueError::duplicate_id;
      }
    }
    entries_.push_back(entry);
    return std::nullopt;
  }

  Result<QueueEntry> extractMin() override {
    Result<QueueEntry> result{};
    if (entries_.empty()) {
      result.error = QueueError::empty_queue;
      return result;
    }

    auto min_it = entries_.begin();
    for (auto it = entries_.begin() + 1; it != entries_.end(); ++it) {
      if (isLessByPriority(*it, *min_it)) {
        min_it = it;
      }
    }

    QueueEntry value = *min_it;
    entries_.erase(min_it);
    result.value = value;
    return result;
  }

  std::optional<QueueError> decreaseKey(PatientId id, PriorityKey new_key) override {
    for (auto& entry : entries_) {
      if (entry.patient_id == id) {
        if (new_key >= entry.priority_key) {
          return QueueError::key_not_decreased;
        }
        entry.priority_key = new_key;
        return std::nullopt;
      }
    }
    return QueueError::missing_id;
  }

  std::optional<QueueError> deletePatient(PatientId id) override {
    for (auto it = entries_.begin(); it != entries_.end(); ++it) {
      if (it->patient_id == id) {
        entries_.erase(it);
        return std::nullopt;
      }
    }
    return QueueError::missing_id;
  }

  std::optional<QueueError> buildHeap(const std::vector<QueueEntry>& entries) override {
    std::unordered_map<PatientId, bool> seen;
    seen.reserve(entries.size());
    for (const auto& entry : entries) {
      if (entry.patient_id == 0 || seen[entry.patient_id]) {
        return QueueError::duplicate_id;
      }
      seen[entry.patient_id] = true;
    }

    entries_ = entries;
    return std::nullopt;
  }

  bool empty() const override { return entries_.empty(); }
  std::size_t size() const override { return entries_.size(); }

 private:
  std::vector<QueueEntry> entries_;
};

using IndexedPriorityQueue = IndexedBinaryMinHeap;
using LazyPriorityQueue = LazyInvalidationBinaryHeap;
using UnsortedPriorityQueue = UnsortedArrayOracle;
using IndexedBinaryMinHeapQueue = IndexedBinaryMinHeap;
using LazyInvalidationPriorityQueue = LazyInvalidationBinaryHeap;
using UnsortedArrayOracleQueue = UnsortedArrayOracle;

}  // namespace triage
