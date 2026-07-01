#include "simulation.h"
#include <stdexcept>

 
ServiceManager::ServiceManager(std::vector<std::unique_ptr<Adjuster>>& adjusters)
    : adjusters_(adjusters)
{}

Adjuster* ServiceManager::findAvailableAdjuster(const std::string& category)
{
    // First check the idle-adjuster queue (FIFO order)
    // We may need to scan because not every queued adjuster can fix every category
    std::queue<int> temp;
    Adjuster* found = nullptr;

    while (!adjusterQueue_.empty()) {
        int aid = adjusterQueue_.front();
        adjusterQueue_.pop();
        if (!found && adjusters_[aid]->canService(category)) {
            found = adjusters_[aid].get();
        } else {
            temp.push(aid);
        }
    }
    adjusterQueue_ = std::move(temp);
    return found;
}

std::optional<double> ServiceManager::machineFailed(Machine& m, double now,
                                                      [[maybe_unused]] std::mt19937& rng)
{
    m.markFailed(now);

    // Try to find an idle adjuster immediately
    Adjuster* adj = findAvailableAdjuster(m.category());
    if (adj) {
        // Adjuster was waiting — pull it off the idle queue (already done above)
        adj->markQueueExit(now);
        adj->markBusy(now);
        m.markQueueEntry(now);
        m.markQueueExit(now);   // no actual wait

        double repairTime = now + m.repairDuration();
        return repairTime;
    } else {
        // Queue the machine
        m.markQueueEntry(now);
        machineQueue_.push(m.id());
        return std::nullopt;
    }
}

ServiceManager::DispatchResult
ServiceManager::repairComplete(Machine& m, Adjuster& a,
                               double now, [[maybe_unused]] std::mt19937& rng)
{
    a.markIdle(now);
    m.markRepaired(now);

    // Schedule the repaired machine's next failure
    double nextFail = m.nextFailureTime(now);

    // Is there a machine in the queue waiting?
    // Scan queue for a machine this adjuster can service
    std::queue<int> temp;
    int nextMachineId = -1;

    while (!machineQueue_.empty()) {
        int mid = machineQueue_.front();
        machineQueue_.pop();
        
        // (Full category-aware matching is handled if needed; basic FIFO used here.)
        if (nextMachineId == -1) {
            nextMachineId = mid;   // take first available (simplified FIFO)
        } else {
            temp.push(mid);
        }
    }
    machineQueue_ = std::move(temp);

    DispatchResult result;
    result.nextFailureTime = nextFail;
    result.nextMachineId   = nextMachineId;

    if (nextMachineId != -1) {
        // Adjuster immediately starts on queued machine — actual duration
        // computed in Simulation using that machine's repairDuration().
        a.markBusy(now);
        result.nextRepairCompleteTime = std::nullopt; // Simulation fills this in
    } else {
        // No machines waiting — adjuster goes idle into adjuster queue
        a.markQueueEntry(now);
        adjusterQueue_.push(a.id());
        result.nextRepairCompleteTime = std::nullopt;
    }

    return result;
}
