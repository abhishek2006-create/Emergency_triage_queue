#ifndef TRIAGE_ADVERSARY_HPP
#define TRIAGE_ADVERSARY_HPP

#include "../src/common/triage_types.hpp"
#include <vector>
#include <cstdint>
#include <cstddef>

namespace triage {
namespace adversary {

enum class Action { Insert, Decrease, Delete, Extract, Build };
struct Event {
    Action action;
    QueueEntry entry;
    PatientId id;
    PriorityKey new_key;
};
struct Workload {
    std::vector<Event> events;
    std::vector<QueueEntry> initial;
};

Workload worstCaseSiftUp(std::size_t n);
Workload shuffledExtracts(std::size_t n, std::uint32_t seed);
Workload repeatedRevisions(std::size_t patients, std::size_t revisions);
Workload repeatedDepartures(std::size_t patients);

struct StarvationRow {
    Timestamp minute;
    PatientId treated_without_aging;
    PatientId treated_linear;
    PatientId treated_stepwise;
    PriorityKey low_linear_urgency;
    PriorityKey low_stepwise_urgency;
};

std::vector<StarvationRow> starvationScenario(
    std::size_t treatments,
    PriorityKey initial_low_severity,
    PriorityKey arriving_high_severity,
    PriorityKey linear_per_minute,
    PriorityKey step_increment,
    Timestamp step_minutes
);

}
}
#endif
