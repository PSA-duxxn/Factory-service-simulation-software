#include "simulation.h"
#include <cassert>
#include <iostream>
#include <stdexcept>

 
static SimConfig makeConfig(int machineCount, int adjusterCount,
                             double mttf = 10.0, double mttr = 2.0,
                             double duration = 500.0)
{
    SimConfig cfg;
    cfg.simDuration = duration;
    cfg.randomSeed  = 42;

    MachineCategory cat;
    cat.name  = "TestMachine";
    cat.count = machineCount;
    cat.mttf  = mttf;
    cat.mttR  = mttr;
    cfg.categories.push_back(cat);

    AdjusterProfile prof;
    prof.name      = "TestAdjuster";
    prof.expertise = {"TestMachine"};
    cfg.adjusterTypes.push_back(prof);
    cfg.adjusterCounts.push_back(adjusterCount);

    return cfg;
}

 
static void test_utilization_between_0_and_1()
{
    Simulation sim(makeConfig(20, 3));
    SimResults res = sim.run();

    assert(res.overallMachineUtilization  >= 0.0 && res.overallMachineUtilization  <= 1.0);
    assert(res.overallAdjusterUtilization >= 0.0 && res.overallAdjusterUtilization <= 1.0);
    std::cout << "  [PASS] utilization_between_0_and_1\n";
}

static void test_more_adjusters_higher_machine_util()
{
    // With very few adjusters machines spend more time down
    Simulation sim_few(makeConfig(50, 1));
    SimResults res_few = sim_few.run();

    Simulation sim_many(makeConfig(50, 20));
    SimResults res_many = sim_many.run();

    assert(res_many.overallMachineUtilization >= res_few.overallMachineUtilization);
    std::cout << "  [PASS] more_adjusters_higher_machine_util\n";
}

static void test_more_adjusters_lower_adjuster_util()
{
    // With many adjusters each adjuster is less busy
    Simulation sim_few(makeConfig(10, 1));
    SimResults res_few = sim_few.run();

    Simulation sim_many(makeConfig(10, 10));
    SimResults res_many = sim_many.run();

    assert(res_few.overallAdjusterUtilization >= res_many.overallAdjusterUtilization);
    std::cout << "  [PASS] more_adjusters_lower_adjuster_util\n";
}

static void test_repairs_completed_positive()
{
    Simulation sim(makeConfig(30, 5, 5.0, 1.0, 200.0));
    SimResults res = sim.run();
    assert(res.totalRepairsCompleted > 0);
    std::cout << "  [PASS] repairs_completed_positive\n";
}

static void test_invalid_mttf_throws()
{
    try {
        SimConfig cfg = makeConfig(5, 1, -1.0, 2.0); // invalid MTTF
        Simulation sim(cfg);
        sim.run(); // should throw
        std::cout << "  [FAIL] invalid_mttf_throws — no exception raised\n";
        assert(false);
    } catch (const std::exception&) {
        std::cout << "  [PASS] invalid_mttf_throws\n";
    }
}

static void test_zero_adjusters_throws()
{
    try {
        SimConfig cfg = makeConfig(10, 0);
        Simulation sim(cfg);
        sim.run();
        std::cout << "  [FAIL] zero_adjusters_throws — no exception raised\n";
        assert(false);
    } catch (const std::exception&) {
        std::cout << "  [PASS] zero_adjusters_throws\n";
    }
}

 
int main()
{
    std::cout << "\nRunning basic simulation tests...\n";
    test_utilization_between_0_and_1();
    test_more_adjusters_higher_machine_util();
    test_more_adjusters_lower_adjuster_util();
    test_repairs_completed_positive();
    test_invalid_mttf_throws();
    test_zero_adjusters_throws();
    std::cout << "All basic tests passed.\n\n";
    return 0;
}
