#include "reporter.h"
#include "simulation.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cmath>

 
static std::string pct(double f) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << (f * 100.0) << "%";
    return ss.str();
}

static std::string bar(double f, int width = 30) {
    int filled = static_cast<int>(std::round(f * width));
    return std::string(filled, '#') + std::string(width - filled, '-');
}

 
void printReport(const SimResults& results)
{
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║        FACTORY SERVICE SIMULATION — RESULTS              ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n\n";

    std::cout << "  Simulation Duration   : " << results.simDuration << " hours\n";
    std::cout << "  Total Repairs Done    : " << results.totalRepairsCompleted << "\n";
    std::cout << "  Avg Repair Wait Time  : "
              << std::fixed << std::setprecision(4)
              << results.avgRepairWaitTime << " hours\n\n";

    // ── Overall utilization ─────────────────────────────────────────
    std::cout << "  ── Overall Utilization ─────────────────────────────────\n";
    std::cout << "  Machine  Util: " << bar(results.overallMachineUtilization)
              << " " << pct(results.overallMachineUtilization) << "\n";
    std::cout << "  Adjuster Util: " << bar(results.overallAdjusterUtilization)
              << " " << pct(results.overallAdjusterUtilization) << "\n\n";

    // ── Per-category machine stats ──────────────────────────────────
    std::cout << "  ── Machine Utilization by Category ─────────────────────\n";
    std::cout << "  " << std::left
              << std::setw(15) << "Category"
              << std::setw(7)  << "Count"
              << std::setw(10) << "Util %"
              << std::setw(12) << "Avg Wait(h)"
              << std::setw(10) << "Failures"
              << "\n";
    std::cout << "  " << std::string(54, '-') << "\n";
    for (const auto& ms : results.machineStats) {
        std::cout << "  " << std::left
                  << std::setw(15) << ms.category
                  << std::setw(7)  << ms.totalMachines
                  << std::setw(10) << pct(ms.avgUtilization)
                  << std::setw(12) << std::fixed << std::setprecision(4) << ms.avgQueueWait
                  << std::setw(10) << ms.totalFailures
                  << "\n";
    }

    // ── Per-type adjuster stats ─────────────────────────────────────
    std::cout << "\n  ── Adjuster Utilization by Type ─────────────────────────\n";
    std::cout << "  " << std::left
              << std::setw(22) << "Adjuster Type"
              << std::setw(7)  << "Count"
              << std::setw(10) << "Util %"
              << std::setw(14) << "Avg Idle(h)"
              << "\n";
    std::cout << "  " << std::string(53, '-') << "\n";
    for (const auto& as : results.adjusterStats) {
        std::cout << "  " << std::left
                  << std::setw(22) << as.adjusterType
                  << std::setw(7)  << as.count
                  << std::setw(10) << pct(as.avgUtilization)
                  << std::setw(14) << std::fixed << std::setprecision(4) << as.avgIdleWait
                  << "\n";
    }
    std::cout << "\n";
}

 
void exportCSV(const SimResults& results, const std::string& path)
{
    std::ofstream f(path);
    if (!f) throw std::runtime_error("Cannot open CSV output: " + path);

    f << "section,name,count,utilization_pct,avg_wait_hours,total_failures\n";

    for (const auto& ms : results.machineStats) {
        f << "machine,"
          << ms.category << ","
          << ms.totalMachines << ","
          << std::fixed << std::setprecision(4) << (ms.avgUtilization * 100.0) << ","
          << ms.avgQueueWait << ","
          << ms.totalFailures << "\n";
    }
    for (const auto& as : results.adjusterStats) {
        f << "adjuster,"
          << as.adjusterType << ","
          << as.count << ","
          << std::fixed << std::setprecision(4) << (as.avgUtilization * 100.0) << ","
          << as.avgIdleWait << ","
          << 0 << "\n";
    }

    std::cout << "  [CSV] Results exported to: " << path << "\n";
}

 
std::vector<SweepPoint> runSweep(const SimConfig& baseConfig,
                                  int minAdj, int maxAdj, int step)
{
    std::vector<SweepPoint> pts;
    for (int n = minAdj; n <= maxAdj; n += step) {
        SimConfig cfg = baseConfig;
        // Scale adjuster counts proportionally to total n
        int total = 0;
        for (int c : cfg.adjusterCounts) total += c;
        if (total == 0) total = 1;

        // Distribute n adjusters proportionally across types
        int assigned = 0;
        for (size_t i = 0; i < cfg.adjusterCounts.size(); ++i) {
            int share = static_cast<int>(std::round(
                (double)cfg.adjusterCounts[i] / total * n));
            cfg.adjusterCounts[i] = std::max(1, share);
            assigned += cfg.adjusterCounts[i];
        }
        // Correct rounding drift on the last type
        if (!cfg.adjusterCounts.empty()) {
            cfg.adjusterCounts.back() += (n - assigned);
            if (cfg.adjusterCounts.back() < 1) cfg.adjusterCounts.back() = 1;
        }

        Simulation sim(cfg);
        SimResults res = sim.run();

        pts.push_back({n,
                       res.overallMachineUtilization,
                       res.overallAdjusterUtilization});
    }
    return pts;
}

void printSweepTable(const std::vector<SweepPoint>& sweep)
{
    std::cout << "\n  ── Adjuster Count Sensitivity Sweep ─────────────────────\n";
    std::cout << "  " << std::left
              << std::setw(12) << "Adjusters"
              << std::setw(20) << "Machine Util %"
              << std::setw(20) << "Adjuster Util %"
              << "\n";
    std::cout << "  " << std::string(52, '-') << "\n";
    for (const auto& pt : sweep) {
        std::cout << "  " << std::left
                  << std::setw(12) << pt.totalAdjusters
                  << std::setw(20) << pct(pt.machineUtilization)
                  << std::setw(20) << pct(pt.adjusterUtilization)
                  << "\n";
    }
    std::cout << "\n";
}

void exportSweepCSV(const std::vector<SweepPoint>& sweep,
                    const std::string& path)
{
    std::ofstream f(path);
    if (!f) throw std::runtime_error("Cannot open sweep CSV: " + path);
    f << "total_adjusters,machine_utilization_pct,adjuster_utilization_pct\n";
    for (const auto& pt : sweep) {
        f << pt.totalAdjusters << ","
          << std::fixed << std::setprecision(4)
          << (pt.machineUtilization * 100.0) << ","
          << (pt.adjusterUtilization * 100.0) << "\n";
    }
    std::cout << "  [CSV] Sweep exported to: " << path << "\n";
}
