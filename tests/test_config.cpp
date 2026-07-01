#include "config_loader.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <cstdio>

static void write_tmp(const std::string& path, const std::string& content) {
    std::ofstream f(path);
    f << content;
}

static void test_load_valid_config()
{
    const std::string path = "/tmp/fsss_test_valid.json";
    write_tmp(path, R"({
        "sim_duration": 1000,
        "random_seed": 99,
        "machine_categories": [
            { "name": "Lathe", "count": 10, "mttf": 8.0, "mttr": 1.0 }
        ],
        "adjuster_types": [
            { "name": "Adj", "expertise": ["Lathe"] }
        ],
        "adjuster_counts": [2]
    })");

    SimConfig cfg = loadConfig(path);
    assert(cfg.simDuration == 1000.0);
    assert(cfg.randomSeed  == 99);
    assert(cfg.categories.size()    == 1);
    assert(cfg.adjusterTypes.size() == 1);
    assert(cfg.adjusterCounts[0]    == 2);
    assert(cfg.categories[0].name  == "Lathe");
    assert(cfg.categories[0].count == 10);
    assert(cfg.categories[0].mttf  == 8.0);
    assert(cfg.categories[0].mttR  == 1.0);

    std::remove(path.c_str());
    std::cout << "  [PASS] load_valid_config\n";
}

static void test_missing_sim_duration_throws()
{
    const std::string path = "/tmp/fsss_test_bad.json";
    write_tmp(path, R"({
        "machine_categories": [
            { "name": "X", "count": 1, "mttf": 5.0, "mttr": 1.0 }
        ],
        "adjuster_types": [{ "name": "A", "expertise": ["X"] }],
        "adjuster_counts": [1]
    })");
    try {
        loadConfig(path);
        assert(false && "should have thrown");
    } catch (const std::exception&) {
        std::cout << "  [PASS] missing_sim_duration_throws\n";
    }
    std::remove(path.c_str());
}

static void test_mismatched_adjuster_counts_throws()
{
    const std::string path = "/tmp/fsss_test_mismatch.json";
    write_tmp(path, R"({
        "sim_duration": 500,
        "machine_categories": [
            { "name": "X", "count": 5, "mttf": 5.0, "mttr": 1.0 }
        ],
        "adjuster_types": [{ "name": "A", "expertise": ["X"] }],
        "adjuster_counts": [1, 2]
    })");
    try {
        loadConfig(path);
        assert(false && "should have thrown");
    } catch (const std::exception&) {
        std::cout << "  [PASS] mismatched_adjuster_counts_throws\n";
    }
    std::remove(path.c_str());
}

static void test_template_round_trip()
{
    const std::string path = "/tmp/fsss_test_template.json";
    saveConfigTemplate(path);
    SimConfig cfg = loadConfig(path);     // should not throw
    assert(cfg.simDuration > 0);
    assert(!cfg.categories.empty());
    std::remove(path.c_str());
    std::cout << "  [PASS] template_round_trip\n";
}

int main()
{
    std::cout << "\nRunning config loader tests...\n";
    test_load_valid_config();
    test_missing_sim_duration_throws();
    test_mismatched_adjuster_counts_throws();
    test_template_round_trip();
    std::cout << "All config tests passed.\n\n";
    return 0;
}
