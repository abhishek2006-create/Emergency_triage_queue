#include "triage_oracle.hpp"
#include "../src/structA/indexed_heap.hpp"
#include "../src/structB/lazy_heap.hpp"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace triage;

struct Action {
    int kind;
    PatientId id;
    PriorityKey key;
    Timestamp arrival;
    std::vector<QueueEntry> initial;
};

static bool equalEntry(const QueueEntry& a, const QueueEntry& b) {
    return a.patient_id == b.patient_id && a.priority_key == b.priority_key && a.arrival_time == b.arrival_time;
}

static bool equalResult(const ExtractResult& x, const ExtractResult& y) {
    return x.ok == y.ok && x.error == y.error && (!x.ok || equalEntry(x.entry, y.entry));
}

static std::string describe(const Action& a) {
    std::ostringstream os;
    if (a.kind == 0) os << "insert " << a.id << " " << a.key << " " << a.arrival;
    else if (a.kind == 1) os << "decreaseKey " << a.id << " " << a.key;
    else if (a.kind == 2) os << "deletePatient " << a.id;
    else if (a.kind == 3) os << "extractMin";
    else {
        os << "buildHeap " << a.initial.size();
        for (std::size_t i = 0; i < a.initial.size(); ++i)
            os << " " << a.initial[i].patient_id << ":" << a.initial[i].priority_key << ":" << a.initial[i].arrival_time;
    }
    return os.str();
}

static bool replay(const std::vector<Action>& actions, std::size_t* failing, std::string* detail, bool checkInvariants) {
    IndexedBinaryMinHeap a;
    LazyBinaryMinHeap b;
    TriageOracle o;
    for (std::size_t i = 0; i < actions.size(); ++i) {
        const Action& x = actions[i];
        bool good = true;
        if (x.kind == 0) {
            QueueEntry e = {x.id, x.key, x.arrival};
            QueueError expected = o.insert(e);
            good = a.insert(e) == expected && b.insert(e) == expected;
        } else if (x.kind == 1) {
            QueueError expected = o.decreaseKey(x.id, x.key);
            good = a.decreaseKey(x.id, x.key) == expected && b.decreaseKey(x.id, x.key) == expected;
        } else if (x.kind == 2) {
            QueueError expected = o.deletePatient(x.id);
            good = a.deletePatient(x.id) == expected && b.deletePatient(x.id) == expected;
        } else if (x.kind == 3) {
            ExtractResult expected = o.extractMin();
            ExtractResult ra = a.extractMin();
            ExtractResult rb = b.extractMin();
            good = equalResult(expected, ra) && equalResult(expected, rb);
        } else {
            QueueError expected = o.buildHeap(x.initial);
            good = a.buildHeap(x.initial) == expected && b.buildHeap(x.initial) == expected;
        }
        if (good) good = a.size() == o.size() && b.size() == o.size() && a.empty() == o.empty() && b.empty() == o.empty();
        if (good && checkInvariants) good = a.validate() && b.validate() && o.validate() && b.physicalSize() <= 2 * b.size();
        if (!good) {
            if (failing) *failing = i;
            if (detail) *detail = describe(x);
            return false;
        }
    }
    return true;
}

static std::vector<Action> shrink(std::vector<Action> failure) {
    std::size_t granularity = 2;
    while (failure.size() >= 2) {
        const std::size_t chunk = (failure.size() + granularity - 1) / granularity;
        bool shortened = false;
        for (std::size_t start = 0; start < failure.size(); start += chunk) {
            const std::size_t end = std::min(start + chunk, failure.size());
            std::vector<Action> candidate;
            candidate.reserve(failure.size() - (end - start));
            candidate.insert(candidate.end(), failure.begin(), failure.begin() + start);
            candidate.insert(candidate.end(), failure.begin() + end, failure.end());
            if (!replay(candidate, 0, 0, true)) {
                failure.swap(candidate);
                granularity = std::max<std::size_t>(2, granularity - 1);
                shortened = true;
                break;
            }
        }
        if (!shortened) {
            if (granularity >= failure.size()) break;
            granularity = std::min(failure.size(), granularity * 2);
        }
    }
    return failure;
}

static Action generate(std::mt19937_64& rng, std::size_t step) {
    Action a;
    a.kind = static_cast<int>(rng() % 100);
    a.id = rng() % 1200;
    a.key = static_cast<PriorityKey>(rng() % 200001) - 100000;
    a.arrival = static_cast<Timestamp>(step % 43200);
    if (a.kind < 29) a.kind = 0;
    else if (a.kind < 55) a.kind = 1;
    else if (a.kind < 74) a.kind = 2;
    else if (a.kind < 97) a.kind = 3;
    else {
        a.kind = 4;
        std::size_t count = rng() % 30;
        for (std::size_t i = 0; i < count; ++i) {
            QueueEntry e = {rng() % 100, static_cast<PriorityKey>(rng() % 1000) - 500, static_cast<Timestamp>(i)};
            a.initial.push_back(e);
        }
    }
    return a;
}

int main(int argc, char** argv) {
    std::size_t operations = 1000000;
    std::uint64_t seed = 20261116;
    if (argc > 1) operations = static_cast<std::size_t>(std::strtoull(argv[1], 0, 10));
    if (argc > 2) seed = static_cast<std::uint64_t>(std::strtoull(argv[2], 0, 10));
    std::mt19937_64 rng(seed);
    std::vector<Action> actions;
    actions.reserve(operations);
    for (std::size_t i = 0; i < operations; ++i) actions.push_back(generate(rng, i));
    std::size_t failing = 0;
    std::string detail;
    if (!replay(actions, &failing, &detail, true)) {
        std::cerr << "FAIL seed=" << seed << " operation=" << failing << " " << detail << "\n";
        actions.resize(failing + 1);
        std::vector<Action> minimal = shrink(actions);
        std::ofstream out("oracle_failure_minimized.txt");
        for (std::size_t i = 0; i < minimal.size(); ++i) out << describe(minimal[i]) << "\n";
        std::cerr << "Minimized failure: " << minimal.size() << " operations in oracle_failure_minimized.txt\n";
        return 1;
    }
    std::cout << "PASS " << operations << " operations seed=" << seed << "; A, B, oracle and invariants agree\n";
    return 0;
}
