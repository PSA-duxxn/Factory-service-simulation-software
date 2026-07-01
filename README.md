# 🏭 Factory Service Simulation Software (FSSS)

[![CI](https://github.com/YOUR_USERNAME/FSSS/actions/workflows/ci.yml/badge.svg)](https://github.com/YOUR_USERNAME/FSSS/actions)

A discrete-event simulation engine written in C++17 that models machine reliability and adjuster workload in a factory environment — helping management determine the **optimum number of adjusters** to employ.

---

## Table of Contents
- [Overview](#overview)
- [Simulation Model](#simulation-model)
- [Features](#features)
- [Project Structure](#project-structure)
- [Building](#building)
- [Usage](#usage)
- [Configuration Format](#configuration-format)
- [Understanding the Output](#understanding-the-output)
- [Running Tests](#running-tests)
- [Contributing](#contributing)

---

## Overview

A factory contains multiple categories of machines (lathes, turning machines, drilling machines, soldering machines, etc.). Each category fails randomly according to an **exponential distribution** parameterised by its **Mean Time To Failure (MTTF)**. A pool of adjusters — each with expertise in one or more machine categories — is dispatched by a **Service Manager** using a unified single queue.

The simulation answers:
- What is the **machine utilization** (fraction of time each machine is running)?
- What is the **adjuster utilization** (fraction of time each adjuster is busy)?
- How do these change as the **number of adjusters** varies?

---

## Simulation Model

### Key entities

| Entity | Description |
|---|---|
| **Machine** | Fails after exponential(MTTF) time; repaired in exponential(MTTR) time |
| **Adjuster** | Has expertise in ≥1 machine categories; services one machine at a time |
| **Service Manager** | Maintains a **unified queue** — at any instant it holds either waiting machines or idle adjusters, never both |

### The Unified Queue

> *"At any given time, one of the two queues will be empty."* — problem statement

When a machine breaks down:
- If an **idle capable adjuster** exists → dispatch immediately (zero wait)
- Otherwise → machine joins the **machine queue**

When a repair completes:
- If **machines are queued** that the adjuster can service → adjuster starts immediately
- Otherwise → adjuster joins the **adjuster queue**

### Statistical distributions

Both inter-failure and repair times follow the **exponential distribution**, which is:
- Memoryless — consistent with the problem's uniform-failure-after-continuous-operation model
- Fully parameterised by a single value (MTTF or MTTR)

---

## Features

- ✅ Discrete-event simulation core (priority-queue driven)
- ✅ Multiple machine categories with independent MTTF / MTTR
- ✅ Multiple adjuster types with different expertise sets
- ✅ Unified single-queue dispatcher (faithful to the problem spec)
- ✅ Category-aware adjuster matching
- ✅ Per-category machine utilization and failure statistics
- ✅ Per-type adjuster utilization and idle-time statistics
- ✅ **Adjuster-count sensitivity sweep** — varies adjuster headcount and plots the utilization curve
- ✅ JSON configuration — no recompilation needed to model a new factory
- ✅ CSV export for further analysis (Excel, Python, R, …)
- ✅ Zero external dependencies — standard C++17 only
- ✅ Cross-platform: Linux, macOS, Windows (MSVC, GCC, Clang)

---

## Project Structure

```
FSSS/
├── CMakeLists.txt              # Top-level build definition
├── README.md
├── .gitignore
├── .github/
│   └── workflows/
│       └── ci.yml              # GitHub Actions CI (Linux + macOS + Windows)
├── include/
│   ├── simulation.h            # Core types, Machine, Adjuster, Simulation
│   ├── config_loader.h         # JSON config loader
│   └── reporter.h              # Output / CSV / sweep
├── src/
│   ├── main.cpp                # CLI entry point
│   ├── simulation.cpp          # Event-loop engine
│   ├── machine.cpp             # Machine state & statistics
│   ├── adjuster.cpp            # Adjuster state & statistics
│   ├── service_manager.cpp     # Unified-queue dispatcher
│   ├── config_loader.cpp       # Hand-rolled JSON parser (no deps)
│   └── reporter.cpp            # Reports, CSV, sweep
├── config/
│   ├── factory_default.json    # Full factory example (360 machines)
│   └── small_factory.json      # Small factory for quick tests
├── tests/
│   ├── CMakeLists.txt
│   ├── test_basic.cpp          # Simulation invariant tests
│   └── test_config.cpp         # Config loader tests
└── scripts/
    ├── build.sh                # Convenience build script
    └── demo.sh                 # End-to-end demo
```

---

## Building

### Prerequisites
- CMake ≥ 3.16
- A C++17-capable compiler: GCC 9+, Clang 10+, MSVC 2019+

### Linux / macOS

```bash
git clone https://github.com/YOUR_USERNAME/FSSS.git
cd FSSS

# Quick build
bash scripts/build.sh

# Or manually
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

### Windows (Developer PowerShell)

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The binary is produced at `build/fsss` (or `build\Release\fsss.exe` on Windows).

---

## Usage

### 1 — Generate a config template

```bash
./build/fsss template my_factory.json
```

Edit `my_factory.json` to describe your factory, then:

### 2 — Run a single simulation

```bash
./build/fsss run config/factory_default.json
./build/fsss run config/factory_default.json --csv results.csv
```

### 3 — Adjuster-count sensitivity sweep

Find the sweet spot between machine utilization and adjuster cost:

```bash
# Vary total adjusters from 3 to 20, step 1
./build/fsss sweep config/factory_default.json 3 20

# Save to CSV for plotting
./build/fsss sweep config/factory_default.json 3 20 --step 1 --csv sweep.csv
```

### 4 — Full demo

```bash
bash scripts/demo.sh
```

---

## Configuration Format

```jsonc
{
  "sim_duration": 2000,          // Simulation length in hours
  "random_seed": 42,             // Reproducibility seed

  "machine_categories": [
    {
      "name":  "Lathe",          // Category identifier
      "count": 200,              // Number of machines of this type
      "mttf":  12.0,             // Mean Time To Failure (hours)
      "mttr":   2.0              // Mean Time To Repair  (hours)
    }
    // ... more categories
  ],

  "adjuster_types": [
    {
      "name": "GeneralAdjuster",
      "expertise": ["Lathe", "Drilling"]   // Must match category names exactly
    }
    // ... more types
  ],

  // Must have same length as adjuster_types
  "adjuster_counts": [10, 5]     // How many adjusters of each type to employ
}
```

> **Tip:** Run `./build/fsss template` to generate a fully populated example.

---

## Understanding the Output

```
  Machine  Util: ████████████████████░░░░░░░░░░  67.42%
  Adjuster Util: █████████████████████████░░░░░  83.17%
```

| Metric | Meaning |
|---|---|
| **Machine Utilization** | % of sim time each machine is *running* (higher = better output) |
| **Adjuster Utilization** | % of sim time each adjuster is *busy* (higher = good use of staff) |
| **Avg Queue Wait** | Average time a machine waits before an adjuster is assigned |
| **Total Failures** | Total breakdown events for that category |

### Interpreting the sweep

| Scenario | Implication |
|---|---|
| Low adjuster count | Machines queue, utilization drops, production suffers |
| High adjuster count | Adjusters idle, utilization falls, payroll waste |
| **Optimum** | Machine util near theoretical max, adjuster util 70–90% |

---

## Running Tests

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DFSSS_BUILD_TESTS=ON
cmake --build build --parallel
cd build && ctest --output-on-failure
```

---

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/my-feature`)
3. Commit your changes with clear messages
4. Ensure all tests pass (`ctest`)
5. Open a pull request

---

## License

MIT © 2024 — see [LICENSE](LICENSE) for details.
