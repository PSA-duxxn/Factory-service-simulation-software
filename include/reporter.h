#pragma once
#include "simulation.h"
#include <string>

 
void printReport(const SimResults& results);

 
void exportCSV(const SimResults& results, const std::string& path);

 
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
