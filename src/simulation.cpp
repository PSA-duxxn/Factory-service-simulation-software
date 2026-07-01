#include "simulation.h"
#include <stdexcept>
#include <iostream>
#include <unordered_map>
#include <cassert>

// ─────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────
namespace {

// Simple unified queue that stores either machine IDs or adjuster IDs
// (never both at the same time), plus a category-aware search.
struct UnifiedQueue {
    enum class Mode { EMPTY, MACHINES, ADJUSTERS };
    Mode mode = Mode::EMPTY;

    std::queue<int> machineQ;
    std::queue<int> adjusterQ;

    void pushMachine(int id) {
        assert(mode != Mode::ADJUSTERS);
        mode = Mode::MACHINES;
        machineQ.push(id);
    }
    void pushAdjuster(int id) {
        assert(mode != Mode::MACHINES);
        mode = Mode::ADJUSTERS;
        adjusterQ.push(id);
    }
    bool hasMachines()  const { return mode == Mode::MACHINES  && !machineQ.empty(); }
    bool hasAdjusters() const { return mode == Mode::ADJUSTERS && !adjusterQ.empty(); }
    bool empty()        const { return machineQ.empty() && adjusterQ.empty(); }

    // Pop front machine
    int popMachine() {
        int id = machineQ.front(); machineQ.pop();
        if (machineQ.empty()) mode = Mode::EMPTY;
        return id;
    }
    // Pop front adjuster
    int popAdjuster() {
        int id = adjusterQ.front(); adjusterQ.pop();
        if (adjusterQ.empty()) mode = Mode::EMPTY;
        return id;
    }
    // Scan adjuster queue for one that can service 'category'; returns -1 if none.
    // Preserves order of remaining adjusters.
    int findAdjuster(const std::string& category,
                     const std::vector<std::unique_ptr<Adjuster>>& adjs) {
        std::queue<int> temp;
        int found = -1;
        while (!adjusterQ.empty()) {
            int aid = adjusterQ.front(); adjusterQ.pop();
            if (found == -1 && adjs[aid]->canService(category)) {
                found = aid;
            } else {
                temp.push(aid);
            }
        }
        adjusterQ = std::move(temp);
        if (adjusterQ.empty()) mode = (found == -1) ? Mode::EMPTY : Mode::EMPTY;
        if (adjusterQ.empty() && found == -1) mode = Mode::EMPTY;
        else if (adjusterQ.empty())           mode = Mode::EMPTY;
        return found;
    }
    // Scan machine queue for one that adjuster can service; returns -1 if none.
    int findMachine(const std::string& /*adjType*/,
                    const Adjuster& adj,
                    const std::vector<std::unique_ptr<Machine>>& machs,
                    const std::unordered_map<int,int>& idToIndex) {
        std::queue<int> temp;
        int found = -1;
        while (!machineQ.empty()) {
            int mid = machineQ.front(); machineQ.pop();
            auto it = idToIndex.find(mid);
            if (it == idToIndex.end()) continue;
            const std::string& cat = machs[it->second]->category();
            if (found == -1 && adj.canService(cat)) {
                found = mid;
            } else {
                temp.push(mid);
            }
        }
        machineQ = std::move(temp);
        if (machineQ.empty()) mode = Mode::EMPTY;
        return found;
    }
};

} // anonymous namespace

// ─────────────────────────────────────────────
//  Simulation
// ─────────────────────────────────────────────
Simulation::Simulation(const SimConfig& cfg)
    : cfg_(cfg)
    , rng_(cfg.randomSeed)
{}

void Simulation::scheduleFailure(int machineId, double when)
{
    if (when <= cfg_.simDuration)
        eventQueue_.push({when, EventType::MACHINE_FAILURE, machineId, -1});
}

void Simulation::scheduleRepairComplete(int machineId, int adjusterId, double when)
{
    if (when <= cfg_.simDuration)
        eventQueue_.push({when, EventType::REPAIR_COMPLETE, machineId, adjusterId});
}

SimResults Simulation::run()
{
    // ── Build machines ──────────────────────────────────────────────
    std::unordered_map<int,int> machineIdToIndex; // machineId → index in machines_
    int machineId = 0;
    for (const auto& cat : cfg_.categories) {
        for (int i = 0; i < cat.count; ++i) {
            machineIdToIndex[machineId] = static_cast<int>(machines_.size());
            machines_.push_back(std::make_unique<Machine>(machineId, cat, rng_));
            ++machineId;
        }
    }

    // ── Build adjusters ─────────────────────────────────────────────
    int adjusterId = 0;
    for (size_t t = 0; t < cfg_.adjusterTypes.size(); ++t) {
        int cnt = (t < cfg_.adjusterCounts.size()) ? cfg_.adjusterCounts[t] : 0;
        for (int i = 0; i < cnt; ++i) {
            adjusters_.push_back(
                std::make_unique<Adjuster>(adjusterId, cfg_.adjusterTypes[t]));
            ++adjusterId;
        }
    }

    if (adjusters_.empty())
        throw std::runtime_error("Simulation requires at least one adjuster.");

    // ── Schedule initial failures ───────────────────────────────────
    for (auto& m : machines_)
        scheduleFailure(m->id(), m->nextFailureTime(0.0));

    // ── Unified queue ───────────────────────────────────────────────
    UnifiedQueue uq;

    // All adjusters start idle; put them in adjuster queue
    for (auto& a : adjusters_) {
        a->markQueueEntry(0.0);
        uq.pushAdjuster(a->id());
    }

    int    totalRepairs   = 0;
    double totalWaitTime  = 0.0;

    // Track per-machine "running since" for utilization accumulation
    std::vector<double> runningStartTime(machines_.size(), 0.0);
    std::vector<double> totalRunTime(machines_.size(), 0.0);

    // ── Event loop ──────────────────────────────────────────────────
    while (!eventQueue_.empty()) {
        Event ev = eventQueue_.top();
        eventQueue_.pop();

        if (ev.time > cfg_.simDuration) break;
        double now = ev.time;

        if (ev.type == EventType::MACHINE_FAILURE) {
            int midx = machineIdToIndex[ev.machineId];
            Machine& m = *machines_[midx];

            // Accumulate run time
            totalRunTime[midx] += now - runningStartTime[midx];

            m.markFailed(now);
            m.markQueueEntry(now);

            if (uq.hasAdjusters()) {
                // Grab a capable adjuster
                int aid = uq.findAdjuster(m.category(), adjusters_);
                if (aid == -1) {
                    // No capable adjuster available → machine must wait
                    uq.pushMachine(m.id());
                } else {
                    Adjuster& a = *adjusters_[aid];
                    a.markQueueExit(now);
                    a.markBusy(now);
                    m.markQueueExit(now);

                    double waitDuration = 0.0; // no wait
                    totalWaitTime += waitDuration;

                    double repairDone = now + m.repairDuration();
                    scheduleRepairComplete(m.id(), a.id(), repairDone);
                }
            } else {
                // Either adjuster queue is empty or queue holds machines
                uq.pushMachine(m.id());
            }

        } else { // REPAIR_COMPLETE
            int midx = machineIdToIndex[ev.machineId];
            Machine& m   = *machines_[midx];
            Adjuster& a  = *adjusters_[ev.adjusterId];

            m.markRepaired(now);
            a.markIdle(now);
            ++totalRepairs;

            // Machine goes back to running
            runningStartTime[midx] = now;
            scheduleFailure(m.id(), m.nextFailureTime(now));

            // Can adjuster immediately take a queued machine?
            if (uq.hasMachines()) {
                int nextMid = uq.findMachine("", a, machines_, machineIdToIndex);
                if (nextMid != -1) {
                    int nmidx = machineIdToIndex[nextMid];
                    Machine& nm = *machines_[nmidx];

                    double queueWait = now - /* entry time tracked in Machine */ 0.0;
                    nm.markQueueExit(now);
                    totalWaitTime += queueWait;

                    a.markBusy(now);
                    double repairDone = now + nm.repairDuration();
                    scheduleRepairComplete(nm.id(), a.id(), repairDone);
                } else {
                    // Adjuster can't service any queued machine type → goes idle
                    a.markQueueEntry(now);
                    uq.pushAdjuster(a.id());
                }
            } else {
                // No machines waiting → adjuster queues as idle
                a.markQueueEntry(now);
                uq.pushAdjuster(a.id());
            }
        }
    }

    // ── Final accumulation for machines still running at sim end ────
    for (size_t i = 0; i < machines_.size(); ++i) {
        if (machines_[i]->isUp())
            totalRunTime[i] += cfg_.simDuration - runningStartTime[i];
    }

    // ── Final accumulation for adjusters still busy at sim end ──────
    for (auto& a : adjusters_) {
        if (a->isBusy())
            a->markIdle(cfg_.simDuration);
    }

    // ── Collect stats ───────────────────────────────────────────────
    // Per-category machine stats
    std::unordered_map<std::string, std::vector<int>> catMachines;
    for (size_t i = 0; i < machines_.size(); ++i)
        catMachines[machines_[i]->category()].push_back(static_cast<int>(i));

    SimResults res;
    res.simDuration        = cfg_.simDuration;
    res.totalRepairsCompleted = totalRepairs;
    res.avgRepairWaitTime  = (totalRepairs > 0)
                              ? totalWaitTime / totalRepairs
                              : 0.0;

    double sumMachineUtil  = 0;
    int    totalMachineCount = 0;

    for (const auto& cat : cfg_.categories) {
        const auto& indices = catMachines[cat.name];
        double utilSum = 0;
        double qwSum   = 0;
        int    failSum = 0;
        for (int idx : indices) {
            utilSum += totalRunTime[idx] / cfg_.simDuration;
            qwSum   += machines_[idx]->totalQueueWait();
            failSum += machines_[idx]->failures();
        }
        int n = static_cast<int>(indices.size());
        MachineStats ms;
        ms.category       = cat.name;
        ms.totalMachines  = n;
        ms.avgUtilization = (n > 0) ? utilSum / n : 0.0;
        ms.avgQueueWait   = (failSum > 0) ? qwSum / failSum : 0.0;
        ms.totalFailures  = failSum;
        res.machineStats.push_back(ms);

        sumMachineUtil  += utilSum;
        totalMachineCount += n;
    }
    res.overallMachineUtilization = (totalMachineCount > 0)
                                    ? sumMachineUtil / totalMachineCount
                                    : 0.0;

    // Per-adjuster-type stats
    std::unordered_map<std::string, std::vector<int>> typeAdjusters;
    for (size_t i = 0; i < adjusters_.size(); ++i)
        typeAdjusters[adjusters_[i]->typeName()].push_back(static_cast<int>(i));

    double sumAdjUtil   = 0;
    int    totalAdjCount = 0;

    for (const auto& profile : cfg_.adjusterTypes) {
        const auto& indices = typeAdjusters[profile.name];
        double utilSum = 0;
        double idleSum = 0;
        for (int idx : indices) {
            utilSum += adjusters_[idx]->totalBusyTime() / cfg_.simDuration;
            idleSum += adjusters_[idx]->totalIdleWait();
        }
        int n = static_cast<int>(indices.size());
        AdjusterStats as;
        as.adjusterType     = profile.name;
        as.count            = n;
        as.avgUtilization   = (n > 0) ? utilSum / n : 0.0;
        as.avgIdleWait      = (n > 0) ? idleSum / n : 0.0;
        res.adjusterStats.push_back(as);

        sumAdjUtil   += utilSum;
        totalAdjCount += n;
    }
    res.overallAdjusterUtilization = (totalAdjCount > 0)
                                     ? sumAdjUtil / totalAdjCount
                                     : 0.0;

    return res;
}
