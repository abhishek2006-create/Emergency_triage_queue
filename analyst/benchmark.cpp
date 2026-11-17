#include "../src/structA/indexed_heap.hpp"
#include "../src/structB/lazy_heap.hpp"
#include "../oracle/triage_oracle.hpp"
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#endif
static std::uint64_t peakBytes() {
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS m; m.cb = sizeof(m); return GetProcessMemoryInfo(GetCurrentProcess(), &m, sizeof(m)) ? static_cast<std::uint64_t>(m.PeakWorkingSetSize) : 0;
#else
    rusage u; if (getrusage(RUSAGE_SELF, &u) != 0) return 0;
#if defined(__APPLE__)
    return static_cast<std::uint64_t>(u.ru_maxrss);
#else
    return static_cast<std::uint64_t>(u.ru_maxrss) * 1024;
#endif
#endif
}
struct Op { int kind; std::uint64_t id; std::int64_t key; };
template<class Q> static bool apply(Q& q, const Op& o, std::uint64_t i) {
    if (o.kind == 0) return q.insert(triage::QueueEntry{o.id, o.key, static_cast<std::int64_t>(i)}) == triage::QueueError::none;
    if (o.kind == 1) return q.decreaseKey(o.id, o.key) == triage::QueueError::none;
    if (o.kind == 2) return q.extractMin().ok;
    return q.deletePatient(o.id) == triage::QueueError::none;
}
int main(int argc, char** argv) {
    if (argc != 4) { std::cerr << "usage: analyst_benchmark operations seed results.csv\n"; return 2; }
    const std::size_t n = static_cast<std::size_t>(std::strtoull(argv[1], 0, 10)); const unsigned seed = static_cast<unsigned>(std::strtoul(argv[2], 0, 10));
    if (n == 0) { std::cerr << "operations must be positive\n"; return 2; }
    std::ofstream out(argv[3]); if (!out) return 2;
    out << "structure,revision_share_pct,seed,repetition,attempted_operations,successful_operations,elapsed_ns,ns_per_attempted_operation,peak_bytes\n";
    const int shares[] = {0,10,20,30,40,50,60,70,80,90};
    for (int st = 0; st < 2; ++st) for (int share : shares) for (int rep = 0; rep < 5; ++rep) {
        std::mt19937 rng(seed + static_cast<unsigned>(share * 1009 + rep * 9176));
        std::vector<Op> ops; ops.reserve(n); std::uint64_t nextId = 1; std::vector<std::uint64_t> active; std::vector<std::int64_t> modelKeys(1,0), modelArrival(1,0);
        const std::size_t targetRevision = n * static_cast<std::size_t>(share) / 100;
        std::size_t revisions = 0;
        for (std::size_t i=0; i<n; ++i) {
            const std::size_t quota = (i + 1) * targetRevision / n;
            if (revisions < targetRevision && revisions < quota && !active.empty()) { std::uint64_t id=active[rng()%active.size()]; std::int64_t key=-static_cast<std::int64_t>(i+1); modelKeys[id]=key; ops.push_back(Op{1,id,key}); ++revisions; }
            else if (active.empty() || rng()%100 < 60) { std::uint64_t id=nextId++; active.push_back(id); std::int64_t key=static_cast<std::int64_t>(rng()%1000000); modelKeys.push_back(key); modelArrival.push_back(static_cast<std::int64_t>(i)); ops.push_back(Op{0,id,key}); }
            else if (rng()%100 < 65) { std::size_t best=0; for (std::size_t j=1;j<active.size();++j) { std::uint64_t x=active[j], y=active[best]; if(modelKeys[x]<modelKeys[y] || (modelKeys[x]==modelKeys[y] && (modelArrival[x]<modelArrival[y] || (modelArrival[x]==modelArrival[y] && x<y)))) best=j; } std::uint64_t id=active[best]; ops.push_back(Op{2,id,0}); active.erase(active.begin()+best); }
            else { std::size_t j=rng()%active.size(); std::uint64_t id=active[j]; ops.push_back(Op{3,id,0}); active.erase(active.begin()+j); }
        }
        if (st == 0) { triage::IndexedBinaryMinHeap q; std::size_t done=0; auto start=std::chrono::steady_clock::now(); for (std::size_t i=0;i<ops.size();++i) { if(apply(q,ops[i],i)) ++done; } auto end=std::chrono::steady_clock::now(); if(!q.validate()) return 3; std::uint64_t ns=std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count(); out << "A,"<<share<<","<<seed<<","<<rep<<","<<ops.size()<<","<<done<<","<<ns<<","<<(ops.empty()?0:ns/ops.size())<<","<<peakBytes()<<"\n"; }
        else { triage::LazyBinaryMinHeap q; std::size_t done=0; auto start=std::chrono::steady_clock::now(); for (std::size_t i=0;i<ops.size();++i) { if(apply(q,ops[i],i)) ++done; } auto end=std::chrono::steady_clock::now(); if(!q.validate()) return 3; std::uint64_t ns=std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count(); out << "B,"<<share<<","<<seed<<","<<rep<<","<<ops.size()<<","<<done<<","<<ns<<","<<(ops.empty()?0:ns/ops.size())<<","<<peakBytes()<<"\n"; }
    }
    std::cout << "Wrote seeded benchmark rows to " << argv[3] << "\n"; return 0;
}
