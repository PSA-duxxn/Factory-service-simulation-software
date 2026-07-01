#pragma once
#include "simulation.h"
#include <string>

/// Loads a SimConfig from a JSON file (hand-rolled parser, no external deps).
/// Throws std::runtime_error on parse failure.
SimConfig loadConfig(const std::string& path);

/// Writes a SimConfig back to a JSON file (useful for generating templates).
void saveConfigTemplate(const std::string& path);

/// Pretty-print the config to stdout for verification.
void printConfig(const SimConfig& cfg);
