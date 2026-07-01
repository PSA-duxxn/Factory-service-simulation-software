#include "simulation.h"
#include <algorithm>

 
Adjuster::Adjuster(int id, const AdjusterProfile& profile)
    : id_(id)
    , typeName_(profile.name)
    , expertise_(profile.expertise)
    , busy_(false)
{}

bool Adjuster::canService(const std::string& category) const
{
    return std::find(expertise_.begin(), expertise_.end(), category)
           != expertise_.end();
}

void Adjuster::markBusy(double now)
{
    busy_      = true;
    busyStart_ = now;
    ++jobsDone_;
}

void Adjuster::markIdle(double now)
{
    busy_          = false;
    totalBusyTime_ += (now - busyStart_);
}

void Adjuster::markQueueEntry(double now)
{
    queueEntryAt_ = now;
}

void Adjuster::markQueueExit(double now)
{
    totalIdleWait_ += (now - queueEntryAt_);
}
