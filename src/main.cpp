#include "simulation.h"
#include "config_loader.h"
#include "reporter.h"

#include <iostream>
#include <string>
#include <stdexcept>
#include <cstring>

 
static void usage(const char* prog)
{
    std::cout << "\nUsage:\n"
              << "  " << prog << " run      <config.json> [--csv <out.csv>]\n"
              << "  " << prog << " sweep    <config.json> <min_adj> <max_adj> [--step N] [--csv <out.csv>]\n"
              << "  " << prog << " template <output.json>\n\n"
              << "Commands:\n"
              << "  run      — run a single simulation from config file\n"
              << "  sweep    — vary adjuster count and show utilization curve\n"
              << "  template — write a sample config JSON to get started\n\n";
}

 
int main(int argc, char* argv[])
{
    if (argc < 2) { usage(argv[0]); return 1; }

    std::string cmd = argv[1];

    try {
       
        if (cmd == "template") {
            std::string outPath = (argc >= 3) ? argv[2] : "config_template.json";
            saveConfigTemplate(outPath);
            std::cout << "Template written to: " << outPath << "\n";
            return 0;
        }

       
        if (cmd == "run") {
            if (argc < 3) { usage(argv[0]); return 1; }
            std::string cfgPath = argv[2];
            std::string csvPath;
            for (int i = 3; i < argc - 1; ++i)
                if (std::strcmp(argv[i], "--csv") == 0) csvPath = argv[i+1];

            SimConfig cfg = loadConfig(cfgPath);
            printConfig(cfg);

            Simulation sim(cfg);
            SimResults res = sim.run();
            printReport(res);

            if (!csvPath.empty()) exportCSV(res, csvPath);
            return 0;
        }

         
        if (cmd == "sweep") {
            if (argc < 5) { usage(argv[0]); return 1; }
            std::string cfgPath = argv[2];
            int minAdj  = std::stoi(argv[3]);
            int maxAdj  = std::stoi(argv[4]);
            int step    = 1;
            std::string csvPath;

            for (int i = 5; i < argc - 1; ++i) {
                if (std::strcmp(argv[i], "--step") == 0) step = std::stoi(argv[i+1]);
                if (std::strcmp(argv[i], "--csv")  == 0) csvPath = argv[i+1];
            }

            SimConfig cfg = loadConfig(cfgPath);
            printConfig(cfg);

            auto sweep = runSweep(cfg, minAdj, maxAdj, step);
            printSweepTable(sweep);

            if (!csvPath.empty()) exportSweepCSV(sweep, csvPath);
            return 0;
        }

        std::cerr << "Unknown command: " << cmd << "\n";
        usage(argv[0]);
        return 1;

    } catch (const std::exception& ex) {
        std::cerr << "\n[ERROR] " << ex.what() << "\n\n";
        return 2;
    }
}
