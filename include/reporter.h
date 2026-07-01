#pragma once
#include "simulation.h"
#include <string>

/// Print a formatted summary report to stdout.
void printReport(const SimResults& results);

/// Export results to a CSV file for further analysis.
void exportCSV(const SimResults& results, const std::string& path);

/// Run a sensitivity sweep: vary adjuster count from minAdj to maxAdj
/// and report machine & adjuster utilization for each count.
struct SweepPoint {
    int    totalAdjusters;
    double machineUtilization;
    double adjusterUtilization;
};

std::vector<SweepPoint> runSweep(const SimConfig& baseConfig,
                                  int minAdj, int maxAdj, int step = 1);

void printSweepTable(const std::vector<SweepPoint>& sweep);
void exportSweepCSV(const std::vector<SweepPoint>& sweep,
                    const std::string& path);
