#ifndef TRIAGE_SIMULATOR_HPP
#define TRIAGE_SIMULATOR_HPP
#include "../src/common/triage_types.hpp"
#include <cstdint>
#include <vector>
namespace triage
{
    namespace simulator
    {
        enum class Aging
        {
            None,
            Linear,
            Stepwise30
        };
        struct PatientSpec
        {
            PatientId id;
            Timestamp arrival;
            int severity;
        };
        struct WaitStat
        {
            int severity;
            Timestamp wait;
        };
        struct Summary
        {
            std::size_t arrivals;
            std::size_t treated;
            std::size_t aging_updates;
            std::vector<WaitStat> waits;
        };
        template <class Queue>
        class Simulator
        {
            Queue queue_;
            Aging aging_;
            PriorityKey rate_;
            PriorityKey step_;
            static const PriorityKey scale_ = 1000000;
            PriorityKey initialKey(const PatientSpec &p) const
            {
                return aging_ == Aging::Linear ? -static_cast<PriorityKey>(p.severity) * scale_ + rate_ * p.arrival : -static_cast<PriorityKey>(p.severity) * scale_;
            }

        public:
            Simulator(Aging a, PriorityKey rate, PriorityKey step) : aging_(a), rate_(rate), step_(step) {}
            Summary run(const std::vector<PatientSpec> &patients, Timestamp service, Timestamp horizon)
            {
                Summary out;
                out.arrivals = 0;
                out.treated = 0;
                out.aging_updates = 0;
                std::size_t next = 0;
                Timestamp now = 0, service_done = -1;
                bool busy = false;
                std::vector<PatientSpec> waiting;
                while (true)
                {
                    Timestamp event = horizon + 1;
                    if (next < patients.size())
                        event = patients[next].arrival;
                    if (busy && service_done < event)
                        event = service_done;
                    if (aging_ == Aging::Stepwise30)
                        for (std::size_t i = 0; i < waiting.size(); ++i)
                            if (waiting[i].id)
                            {
                                Timestamp threshold = waiting[i].arrival + ((now - waiting[i].arrival) / 30 + 1) * 30;
                                if (threshold < event)
                                    event = threshold;
                            }
                    if (event > horizon)
                        break;
                    now = event;
                    while (next < patients.size() && patients[next].arrival == now)
                    {
                        PatientSpec p = patients[next++];
                        if (queue_.insert(QueueEntry{p.id, initialKey(p), p.arrival}) == QueueError::none)
                        {
                            waiting.push_back(p);
                            ++out.arrivals;
                        }
                    }
                    if (aging_ == Aging::Stepwise30)
                        for (std::size_t i = 0; i < waiting.size(); ++i)
                            if (waiting[i].id && now > waiting[i].arrival && (now - waiting[i].arrival) % 30 == 0)
                            {
                                Timestamp steps = (now - waiting[i].arrival) / 30;
                                if (queue_.decreaseKey(waiting[i].id, -static_cast<PriorityKey>(waiting[i].severity) * scale_ - step_ * steps) == QueueError::none)
                                    ++out.aging_updates;
                            }
                    if (busy && service_done == now)
                    {
                        ExtractResult r = queue_.extractMin();
                        if (r.ok)
                        {
                            for (std::size_t i = 0; i < waiting.size(); ++i)
                                if (waiting[i].id == r.entry.patient_id)
                                {
                                    out.waits.push_back(WaitStat{waiting[i].severity, now - waiting[i].arrival});
                                    waiting[i].id = 0;
                                    break;
                                }
                            ++out.treated;
                        }
                        busy = false;
                    }
                    if (!busy && !queue_.empty())
                    {
                        busy = true;
                        service_done = now + service;
                    }
                    if (next >= patients.size() && !busy && queue_.empty())
                        break;
                }
                return out;
            }
        };
    }
}
#endif
