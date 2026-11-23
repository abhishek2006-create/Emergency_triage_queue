#include "adversary.hpp"
#include <algorithm>
#include <random>
#include <stdexcept>
#include <limits>

namespace triage {
namespace adversary {

Workload worstCaseSiftUp(std::size_t n) {
    Workload w;
    w.events.reserve(n);
    if (n > static_cast<std::size_t>(std::numeric_limits<PriorityKey>::max())) throw std::invalid_argument("n too large");
    for (std::size_t i = 0; i < n; ++i) {
        QueueEntry entry = {static_cast<PatientId>(i + 1), -static_cast<PriorityKey>(i), static_cast<Timestamp>(i)};
        w.events.push_back(Event{Action::Insert, entry, entry.patient_id, 0});
    }
    return w;
}

Workload shuffledExtracts(std::size_t n, std::uint32_t seed) {
    Workload w;
    if (n > static_cast<std::size_t>(std::numeric_limits<PriorityKey>::max())) throw std::invalid_argument("n too large");
    w.initial.reserve(n);
    for (std::size_t i = 0; i < n; ++i)
        w.initial.push_back(QueueEntry{static_cast<PatientId>(i + 1), static_cast<PriorityKey>(i + 1), static_cast<Timestamp>(i)});
    std::mt19937 rng(seed);
    std::shuffle(w.initial.begin(), w.initial.end(), rng);
    w.events.reserve(n);
    for (std::size_t i = 0; i < n; ++i)
        w.events.push_back(Event{Action::Extract, QueueEntry{}, 0, 0});
    return w;
}

Workload repeatedRevisions(std::size_t patients, std::size_t revisions) {
    if (patients == 0) throw std::invalid_argument("patients must be positive");
    if (revisions > 1000000000ULL) throw std::invalid_argument("too many revisions");
    Workload w;
    w.initial.reserve(patients);
    w.events.reserve(revisions);
    for (std::size_t i = 0; i < patients; ++i)
        w.initial.push_back(QueueEntry{static_cast<PatientId>(i + 1), 1000000001LL, static_cast<Timestamp>(i)});
    for (std::size_t j = 0; j < revisions; ++j) {
        PatientId id = static_cast<PatientId>(j % patients + 1);
        PriorityKey key = 1000000000LL - static_cast<PriorityKey>(j);
        w.events.push_back(Event{Action::Decrease, QueueEntry{}, id, key});
    }
    return w;
}

Workload repeatedDepartures(std::size_t patients) {
    Workload w;
    w.initial.reserve(patients);
    w.events.reserve(patients);
    for (std::size_t i = 0; i < patients; ++i) {
        w.initial.push_back(QueueEntry{static_cast<PatientId>(i + 1), static_cast<PriorityKey>(i), static_cast<Timestamp>(i)});
        w.events.push_back(Event{Action::Delete, QueueEntry{}, static_cast<PatientId>(i + 1), 0});
    }
    return w;
}

static PriorityKey urgency(const QueueEntry& e, Timestamp now, int policy, PriorityKey linear, PriorityKey step, Timestamp step_min) {
    if (policy == 0) return e.priority_key;
    if (policy == 1) return e.priority_key + linear * (now - e.arrival_time);
    return e.priority_key + step * ((now - e.arrival_time) / step_min);
}

static PatientId treat(std::vector<QueueEntry>& waiting, Timestamp now, int policy, PriorityKey linear, PriorityKey step, Timestamp step_min) {
    if (waiting.empty()) return 0;
    std::size_t best = 0;
    for (std::size_t i = 1; i < waiting.size(); ++i) {
        PriorityKey a = urgency(waiting[i], now, policy, linear, step, step_min);
        PriorityKey b = urgency(waiting[best], now, policy, linear, step, step_min);
        if (a > b || (a == b && (waiting[i].arrival_time < waiting[best].arrival_time ||
            (waiting[i].arrival_time == waiting[best].arrival_time && waiting[i].patient_id < waiting[best].patient_id))))
            best = i;
    }
    PatientId id = waiting[best].patient_id;
    waiting.erase(waiting.begin() + static_cast<std::ptrdiff_t>(best));
    return id;
}

std::vector<StarvationRow> starvationScenario(std::size_t treatments, PriorityKey low, PriorityKey high, PriorityKey linear, PriorityKey step, Timestamp step_minutes) {
    if (step_minutes <= 0 || linear <= 0 || step <= 0 || high <= low) throw std::invalid_argument("invalid aging inputs");
    if (treatments > 1000000) throw std::invalid_argument("too many treatments");
    std::vector<QueueEntry> waiting[3];
    for (int p = 0; p < 3; ++p) waiting[p].push_back(QueueEntry{1, low, 0});
    std::vector<StarvationRow> rows;
    rows.reserve(treatments);
    for (std::size_t k = 1; k <= treatments; ++k) {
        Timestamp now = static_cast<Timestamp>(k * step_minutes);
        QueueEntry incoming = {static_cast<PatientId>(k + 1), high, now};
        for (int p = 0; p < 3; ++p) waiting[p].push_back(incoming);
        StarvationRow row;
        row.minute = now;
        row.low_linear_urgency = low + linear * now;
        row.low_stepwise_urgency = low + step * (now / step_minutes);
        row.treated_without_aging = treat(waiting[0], now, 0, linear, step, step_minutes);
        row.treated_linear = treat(waiting[1], now, 1, linear, step, step_minutes);
        row.treated_stepwise = treat(waiting[2], now, 2, linear, step, step_minutes);
        rows.push_back(row);
    }
    return rows;
}

}
}
