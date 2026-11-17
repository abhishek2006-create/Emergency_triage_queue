#include "adversary.hpp"
#include "../src/structA/indexed_heap.hpp"
#include "../src/structB/lazy_heap.hpp"
#include "../oracle/triage_oracle.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace triage;
using namespace triage::adversary;

class CountedHeap {
    std::vector<QueueEntry> a_;
public:
    std::uint64_t comparisons = 0;
    std::uint64_t swaps = 0;
    void push(const QueueEntry& e) {
        a_.push_back(e);
        std::size_t i = a_.size() - 1;
        while (i > 0) {
            std::size_t p = (i - 1) / 2;
            ++comparisons;
            if (!entryLess(a_[i], a_[p])) break;
            std::swap(a_[i], a_[p]);
            ++swaps;
            i = p;
        }
    }
    void build(const std::vector<QueueEntry>& input) {
        a_ = input;
        comparisons = swaps = 0;
        for (std::size_t i = a_.size() / 2; i > 0; --i) down(i - 1);
    }
    void down(std::size_t i) {
        const std::size_t n = a_.size();
        while (i < n) {
            std::size_t best = i;
            if (i <= (n - 1) / 2) {
                std::size_t left = 2 * i + 1;
                std::size_t right = left + 1;
                if (left < n) { ++comparisons; if (entryLess(a_[left], a_[best])) best = left; }
                if (right < n) { ++comparisons; if (entryLess(a_[right], a_[best])) best = right; }
            }
            if (best == i) break;
            std::swap(a_[i], a_[best]);
            ++swaps;
            i = best;
        }
    }
    QueueEntry pop() {
        QueueEntry answer = a_.front();
        a_.front() = a_.back();
        a_.pop_back();
        if (!a_.empty()) down(0);
        return answer;
    }
    bool empty() const { return a_.empty(); }
};

static void require(bool b, const std::string& message) {
    if (!b) throw std::runtime_error(message);
}

static void verifyWorkload(const Workload& w, bool oracleEnabled) {
    IndexedBinaryMinHeap a;
    LazyBinaryMinHeap b;
    TriageOracle o;
    require(a.buildHeap(w.initial) == QueueError::none, "A initial build");
    require(b.buildHeap(w.initial) == QueueError::none, "B initial build");
    if (oracleEnabled) require(o.buildHeap(w.initial) == QueueError::none, "oracle initial build");
    for (std::size_t i = 0; i < w.events.size(); ++i) {
        const Event& e = w.events[i];
        QueueError ea = QueueError::none, eb = QueueError::none, eo = QueueError::none;
        ExtractResult ra = {false, QueueEntry{}, QueueError::none};
        ExtractResult rb = ra, ro = ra;
        if (e.action == Action::Insert) {
            ea = a.insert(e.entry); eb = b.insert(e.entry);
            if (oracleEnabled) eo = o.insert(e.entry);
        } else if (e.action == Action::Decrease) {
            ea = a.decreaseKey(e.id, e.new_key); eb = b.decreaseKey(e.id, e.new_key);
            if (oracleEnabled) eo = o.decreaseKey(e.id, e.new_key);
        } else if (e.action == Action::Delete) {
            ea = a.deletePatient(e.id); eb = b.deletePatient(e.id);
            if (oracleEnabled) eo = o.deletePatient(e.id);
        } else if (e.action == Action::Extract) {
            ra = a.extractMin(); rb = b.extractMin();
            if (oracleEnabled) ro = o.extractMin();
            require(ra.ok == rb.ok && ra.error == rb.error, "extract result A vs B");
            if (ra.ok) require(ra.entry.patient_id == rb.entry.patient_id && ra.entry.priority_key == rb.entry.priority_key, "extract entry A vs B");
            if (oracleEnabled) {
                require(ra.ok == ro.ok && ra.error == ro.error, "extract result oracle");
                if (ra.ok) require(ra.entry.patient_id == ro.entry.patient_id && ra.entry.priority_key == ro.entry.priority_key, "extract entry oracle");
            }
        }
        if (e.action != Action::Extract) require(ea == eb && (!oracleEnabled || ea == eo), "operation errors differ");
        require(a.size() == b.size() && (!oracleEnabled || a.size() == o.size()), "sizes differ");
        if (oracleEnabled || i % 10000 == 0) require(a.validate() && b.validate() && (!oracleEnabled || o.validate()), "invariant failure");
        require(b.physicalSize() <= 2 * b.size(), "stale limit exceeded");
    }
    require(a.validate() && b.validate(), "final invariants");
}

int main(int argc, char** argv) {
    try {
        std::size_t n = argc > 1 ? static_cast<std::size_t>(std::strtoull(argv[1], 0, 10)) : 100000;
        unsigned seed = argc > 2 ? static_cast<unsigned>(std::strtoul(argv[2], 0, 10)) : 20261116U;
        std::string output = argc > 3 ? argv[3] : "adversary_results.csv";
        require(n >= 16 && n <= 1000000, "n must be 16..1000000");
        std::ofstream csv(output.c_str());
        require(csv.good(), "cannot open CSV file");
        csv << "scenario,n,seed,comparisons,swaps,live,physical,peak_physical,compaction_events,low_patient_treatment_minute\n";

        Workload rising = worstCaseSiftUp(n);
        CountedHeap counted;
        for (std::size_t i = 0; i < rising.events.size(); ++i) counted.push(rising.events[i].entry);
        std::uint64_t expectedSwaps = 0;
        for (std::size_t i = 1; i < n; ++i) {
            std::size_t k = i;
            while (k > 0) { ++expectedSwaps; k = (k - 1) / 2; }
        }
        require(counted.swaps == expectedSwaps, "sift-up attack did not force full ancestor path");
        csv << "sift_up," << n << ',' << seed << ',' << counted.comparisons << ',' << counted.swaps << ",0,0,0,0,0\n";
        std::cout << "PASS sift_up swaps=" << counted.swaps << " comparisons=" << counted.comparisons << '\n';
        verifyWorkload(worstCaseSiftUp(std::min<std::size_t>(n, 2000)), true);

        Workload extraction = shuffledExtracts(n, seed);
        counted.build(extraction.initial);
        std::uint64_t buildComparisons = counted.comparisons;
        counted.comparisons = counted.swaps = 0;
        PriorityKey prev = 0;
        while (!counted.empty()) {
            QueueEntry e = counted.pop();
            require(e.priority_key > prev, "extract order incorrect");
            prev = e.priority_key;
        }
        csv << "extract_shuffled," << n << ',' << seed << ',' << counted.comparisons << ',' << counted.swaps << ",0,0,0,0,0\n";
        std::cout << "PASS shuffled_extraction build_comparisons=" << buildComparisons << " extract_comparisons=" << counted.comparisons << '\n';
        verifyWorkload(shuffledExtracts(std::min<std::size_t>(n, 2000), seed), true);

        std::size_t patients = std::max<std::size_t>(1, n / 20);
        Workload revisions = repeatedRevisions(patients, n);
        LazyBinaryMinHeap b;
        IndexedBinaryMinHeap a;
        require(a.buildHeap(revisions.initial) == QueueError::none && b.buildHeap(revisions.initial) == QueueError::none, "revision setup");
        std::size_t peak = b.physicalSize();
        std::size_t resets = 0;
        std::size_t prevPhysical = peak;
        for (std::size_t i = 0; i < revisions.events.size(); ++i) {
            const Event& e = revisions.events[i];
            require(a.decreaseKey(e.id, e.new_key) == QueueError::none, "A revision failed");
            require(b.decreaseKey(e.id, e.new_key) == QueueError::none, "B revision failed");
            std::size_t physical = b.physicalSize();
            if (physical < prevPhysical) ++resets;
            if (physical > peak) peak = physical;
            prevPhysical = physical;
            require(b.physicalSize() <= 2 * b.size(), "B compaction bound");
        }
        require(resets > 0, "B compaction never triggered");
        require(a.validate() && b.validate(), "revisions invalid final heap");
        csv << "revisions," << n << ',' << seed << ",0,0," << b.size() << ',' << b.physicalSize() << ',' << peak << ',' << resets << ",0\n";
        std::cout << "PASS revisions compact_events=" << resets << " peak_physical=" << peak << " live=" << b.size() << '\n';
        verifyWorkload(repeatedRevisions(100, 1000), true);

        Workload departures = repeatedDepartures(std::min<std::size_t>(n, 20000));
        verifyWorkload(departures, false);
        csv << "departures," << departures.initial.size() << ',' << seed << ",0,0,0,0,0,0,0\n";
        std::cout << "PASS departures count=" << departures.initial.size() << '\n';

        std::vector<StarvationRow> rows = starvationScenario(120, 1, 10, 1, 2, 30);
        Timestamp linearMinute = -1, stepMinute = -1;
        for (std::size_t i = 0; i < rows.size(); ++i) {
            require(rows[i].treated_without_aging != 1, "unaged low patient unexpectedly treated");
            if (rows[i].treated_linear == 1) linearMinute = rows[i].minute;
            if (rows[i].treated_stepwise == 1) stepMinute = rows[i].minute;
        }
        require(linearMinute > 0 && stepMinute > 0, "aging failed to prevent sample starvation");
        csv << "starvation_none," << rows.size() << ',' << seed << ",0,0,0,0,0,0,-1\n";
        csv << "starvation_linear," << rows.size() << ',' << seed << ",0,0,0,0,0,0," << linearMinute << '\n';
        csv << "starvation_step," << rows.size() << ',' << seed << ",0,0,0,0,0,0," << stepMinute << '\n';
        std::cout << "PASS starvation without=never linear=" << linearMinute << "min step=" << stepMinute << "min\n";
        std::cout << "PASS all adversarial scenarios seed=" << seed << " csv=" << output << '\n';
    } catch (const std::exception& e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
    return 0;
}
