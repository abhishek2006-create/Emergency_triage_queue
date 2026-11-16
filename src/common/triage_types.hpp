#ifndef TRIAGE_TYPES_HPP
#define TRIAGE_TYPES_HPP
#include <cstdint>
#include <cstddef>
namespace triage
{
    typedef std::uint64_t PatientId;
    typedef std::int64_t PriorityKey;
    typedef std::int64_t Timestamp;
    struct QueueEntry
    {
        PatientId patient_id;
        PriorityKey priority_key;
        Timestamp arrival_time;
    };
    enum class QueueError
    {
        none,
        duplicate_id,
        missing_id,
        empty_queue,
        key_not_decreased
    };
    struct ExtractResult
    {
        bool ok;
        QueueEntry entry;
        QueueError error;
    };
    inline bool entryLess(const QueueEntry &a, const QueueEntry &b)
    {
        if (a.priority_key != b.priority_key)
            return a.priority_key < b.priority_key;
        if (a.arrival_time != b.arrival_time)
            return a.arrival_time < b.arrival_time;
        return a.patient_id < b.patient_id;
    }
}
#endif
