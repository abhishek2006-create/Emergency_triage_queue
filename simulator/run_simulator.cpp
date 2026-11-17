#include "simulator.hpp"
#include "../src/structA/indexed_heap.hpp"
#include "../src/structB/lazy_heap.hpp"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
int main(int argc, char **argv)
{
    unsigned seed = argc > 1 ? static_cast<unsigned>(std::strtoul(argv[1], 0, 10)) : 20261116u;
    unsigned simulation_days = argc > 3 ? static_cast<unsigned>(std::strtoul(argv[3], 0, 10)) : 30u;
    if (simulation_days == 0)
    {
        std::cerr << "simulation days must be greater than zero\n";
        return 2;
    }
    const triage::Timestamp horizon = static_cast<triage::Timestamp>(simulation_days) * 24 * 60;
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> gap(1, 12), severity(1, 10);
    std::vector<triage::simulator::PatientSpec> patients;
    triage::Timestamp t = 0;
    for (triage::PatientId id = 1;; ++id)
    {
        t += gap(rng);
        if (t > horizon)
            break;
        patients.push_back(triage::simulator::PatientSpec{id, t, severity(rng)});
    }
    triage::simulator::Simulator<triage::IndexedBinaryMinHeap> linear(triage::simulator::Aging::Linear, 100, 200);
    triage::simulator::Simulator<triage::LazyBinaryMinHeap> step(triage::simulator::Aging::Stepwise30, 100, 200);
    triage::simulator::Simulator<triage::IndexedBinaryMinHeap> baseline(triage::simulator::Aging::None, 100, 200);
    triage::simulator::Summary a = linear.run(patients, 4, horizon);
    triage::simulator::Summary b = step.run(patients, 4, horizon);
    triage::simulator::Summary c = baseline.run(patients, 4, horizon);
    std::ofstream csv(argc > 2 ? argv[2] : "results/simulator.csv");
    if (!csv)
    {
        std::cerr << "cannot open simulator results CSV\n";
        return 2;
    }
    csv << "policy,severity,count,mean_wait,median_wait,p95_wait,max_wait,aging_updates,arrivals,treated\n";
    const triage::simulator::Summary *summaries[] = {&c, &a, &b};
    const char *names[] = {"none", "linear", "stepwise30"};
    for (int policy = 0; policy < 3; ++policy)
        for (int s = 1; s <= 10; ++s)
        {
            std::vector<triage::Timestamp> waits;
            long long total = 0;
            for (std::size_t i = 0; i < summaries[policy]->waits.size(); ++i)
                if (summaries[policy]->waits[i].severity == s)
                {
                    waits.push_back(summaries[policy]->waits[i].wait);
                    total += summaries[policy]->waits[i].wait;
                }
            if (waits.empty())
                continue;
            std::sort(waits.begin(), waits.end());
            csv << names[policy] << ',' << s << ',' << waits.size() << ',' << (total / static_cast<long long>(waits.size())) << ',' << waits[waits.size() / 2] << ',' << waits[(waits.size() - 1) * 95 / 100] << ',' << waits.back() << ',' << summaries[policy]->aging_updates << ',' << summaries[policy]->arrivals << ',' << summaries[policy]->treated << '\n';
        }
    std::cout << "seed=" << seed << " simulation_days=" << simulation_days << " horizon_minutes=" << horizon << " arrivals=" << a.arrivals << "\n";
    std::cout << "linear_treated=" << a.treated << " stepwise_treated=" << b.treated << " baseline_treated=" << c.treated << " step_updates=" << b.aging_updates << "\n";
    for (int s = 1; s <= 10; ++s)
    {
        std::vector<triage::Timestamp> waits;
        for (std::size_t i = 0; i < b.waits.size(); ++i)
            if (b.waits[i].severity == s)
                waits.push_back(b.waits[i].wait);
        std::sort(waits.begin(), waits.end());
        if (!waits.empty())
            std::cout << "step_severity=" << s << " n=" << waits.size() << " median=" << waits[waits.size() / 2] << " p95=" << waits[(waits.size() - 1) * 95 / 100] << " max=" << waits.back() << "\n";
    }
    std::cout << "csv=" << (argc > 2 ? argv[2] : "results/simulator.csv") << "\n";
    std::size_t expected = 0;
    for (std::size_t i = 0; i < patients.size(); ++i)
        if (patients[i].arrival <= horizon)
            ++expected;
    return (a.arrivals == expected && a.arrivals == b.arrivals && b.arrivals == c.arrivals && a.treated == b.treated && b.treated == c.treated) ? 0 : 1;
}
