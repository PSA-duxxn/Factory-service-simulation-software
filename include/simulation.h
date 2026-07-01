#pragma once
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <functional>
#include <memory>
#include <random>
#include <optional>

// ─────────────────────────────────────────────
//  Forward declarations
// ─────────────────────────────────────────────
struct MachineCategory;
struct AdjusterProfile;
class Machine;
class Adjuster;
class ServiceManager;
class Simulation;

// ─────────────────────────────────────────────
//  Data-transfer types (used in config & reports)
// ─────────────────────────────────────────────
struct MachineCategory {
    std::string name;
    int         count;      // number of machines in factory
    double      mttf;       // mean time to failure (hours)
    double      mttR;       // mean time to repair  (hours)
};

struct AdjusterProfile {
    std::string              name;
    std::vector<std::string> expertise; // category names this adjuster can service
};

struct SimConfig {
    double                       simDuration;    // total sim time (hours)
    std::vector<MachineCategory> categories;
    std::vector<AdjusterProfile> adjusterTypes;
    std::vector<int>             adjusterCounts; // count per AdjusterProfile index
    unsigned int                 randomSeed;
};

// ─────────────────────────────────────────────
//  Results
// ─────────────────────────────────────────────
struct MachineStats {
    std::string category;
    int         totalMachines;
    double      avgUtilization;   // fraction of time running
    double      avgQueueWait;     // avg time in repair queue
    int         totalFailures;
};

struct AdjusterStats {
    std::string adjusterType;
    int         count;
    double      avgUtilization;   // fraction of time busy
    double      avgIdleWait;      // avg time spent idle before assignment
};

struct SimResults {
    double                     simDuration;
    double                     overallMachineUtilization;
    double                     overallAdjusterUtilization;
    std::vector<MachineStats>  machineStats;
    std::vector<AdjusterStats> adjusterStats;
    int                        totalRepairsCompleted;
    double                     avgRepairWaitTime;
};

// ─────────────────────────────────────────────
//  Event types for the event-driven core
// ─────────────────────────────────────────────
enum class EventType {
    MACHINE_FAILURE,
    REPAIR_COMPLETE
};

struct Event {
    double    time;
    EventType type;
    int       machineId;
    int       adjusterId; // -1 if not yet assigned

    bool operator>(const Event& o) const { return time > o.time; }
};

// ─────────────────────────────────────────────
//  Machine
// ─────────────────────────────────────────────
class Machine {
public:
    Machine(int id, const MachineCategory& cat, std::mt19937& rng);

    int         id()       const { return id_; }
    std::string category() const { return category_; }
    bool        isUp()     const { return up_; }

    double nextFailureTime(double now);
    double repairDuration();

    // Statistics accumulators
    void markFailed(double now);
    void markRepaired(double now);
    void markQueueEntry(double now);
    void markQueueExit(double now);

    double totalUpTime()    const { return totalUpTime_; }
    double totalDownTime()  const { return totalDownTime_; }
    double totalQueueWait() const { return totalQueueWait_; }
    int    failures()       const { return failures_; }

private:
    int         id_;
    std::string category_;
    double      mttf_;
    double      mttR_;
    bool        up_;

    double failedAt_      = 0;
    double queueEntryAt_  = 0;
    double totalUpTime_   = 0;
    double totalDownTime_ = 0;
    double totalQueueWait_= 0;
    int    failures_      = 0;

    std::exponential_distribution<double> failureDist_;
    std::exponential_distribution<double> repairDist_;
    std::mt19937& rng_;
};

// ─────────────────────────────────────────────
//  Adjuster
// ─────────────────────────────────────────────
class Adjuster {
public:
    Adjuster(int id, const AdjusterProfile& profile);

    int         id()       const { return id_; }
    std::string typeName() const { return typeName_; }
    bool        isBusy()   const { return busy_; }
    bool        canService(const std::string& category) const;

    void markBusy(double now);
    void markIdle(double now);
    void markQueueEntry(double now);
    void markQueueExit(double now);

    double totalBusyTime() const { return totalBusyTime_; }
    double totalIdleWait() const { return totalIdleWait_; }
    int    jobsDone()      const { return jobsDone_; }

private:
    int                      id_;
    std::string              typeName_;
    std::vector<std::string> expertise_;
    bool                     busy_ = false;

    double busyStart_     = 0;
    double queueEntryAt_  = 0;
    double totalBusyTime_ = 0;
    double totalIdleWait_ = 0;
    int    jobsDone_      = 0;
};

// ─────────────────────────────────────────────
//  ServiceManager  (single-queue dispatcher)
// ─────────────────────────────────────────────
class ServiceManager {
public:
    explicit ServiceManager(std::vector<std::unique_ptr<Adjuster>>& adjusters);

    // Called when a machine breaks down.
    // Returns repair-complete event time if an adjuster was assigned immediately,
    // or std::nullopt if the machine is queued.
    std::optional<double> machineFailed(Machine& m, double now,
                                        std::mt19937& rng);

    // Called when a repair completes.
    // Returns next failure event time for the machine just fixed,
    // and optionally a repair-complete event for the next queued machine.
    struct DispatchResult {
        double                nextFailureTime;
        std::optional<double> nextRepairCompleteTime;
        int                   nextMachineId;   // valid iff nextRepairCompleteTime has value
    };
    DispatchResult repairComplete(Machine& m, Adjuster& a,
                                  double now, std::mt19937& rng);

private:
    std::vector<std::unique_ptr<Adjuster>>& adjusters_;

    // The unified queue holds either waiting machines or idle adjusters
    // (never both at the same time)
    std::queue<int> machineQueue_;   // machine ids waiting for repair
    std::queue<int> adjusterQueue_;  // adjuster ids waiting for work

    Adjuster* findAvailableAdjuster(const std::string& category);
};

// ─────────────────────────────────────────────
//  Simulation engine
// ─────────────────────────────────────────────
class Simulation {
public:
    explicit Simulation(const SimConfig& cfg);

    SimResults run();

private:
    SimConfig cfg_;
    std::mt19937 rng_;

    std::vector<std::unique_ptr<Machine>>  machines_;
    std::vector<std::unique_ptr<Adjuster>> adjusters_;

    std::priority_queue<Event,
                        std::vector<Event>,
                        std::greater<Event>> eventQueue_;

    void scheduleFailure(int machineId, double when);
    void scheduleRepairComplete(int machineId, int adjusterId, double when);

    SimResults collectResults();
};
