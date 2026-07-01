#include "simulation.h"
#include <stdexcept>

// ─────────────────────────────────────────────
//  Machine
// ─────────────────────────────────────────────
Machine::Machine(int id, const MachineCategory& cat, std::mt19937& rng)
    : id_(id)
    , category_(cat.name)
    , mttf_(cat.mttf)
    , mttR_(cat.mttR)
    , up_(true)
    , failureDist_(1.0 / cat.mttf)
    , repairDist_(1.0 / cat.mttR)
    , rng_(rng)
{
    if (cat.mttf <= 0 || cat.mttR <= 0)
        throw std::invalid_argument("MTTF and MTTR must be positive for category: " + cat.name);
}

double Machine::nextFailureTime(double now)
{
    return now + failureDist_(rng_);
}

double Machine::repairDuration()
{
    return repairDist_(rng_);
}

void Machine::markFailed(double now)
{
    up_         = false;
    failedAt_   = now;
    ++failures_;
    totalUpTime_ += (now - failedAt_); // accumulated while it was running
    // Note: totalUpTime_ is adjusted properly in markRepaired
}

void Machine::markQueueEntry(double now)
{
    queueEntryAt_ = now;
}

void Machine::markQueueExit(double now)
{
    totalQueueWait_ += (now - queueEntryAt_);
}

void Machine::markRepaired(double now)
{
    totalDownTime_ += (now - failedAt_);
    up_ = true;
}
