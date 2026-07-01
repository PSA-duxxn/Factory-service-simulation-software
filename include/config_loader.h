#pragma once
#include "simulation.h"
#include <string>

/// Loads a SimConfig from a JSON file 
/// Throws error on parse failure.
SimConfig loadConfig(const std::string& path);

/// Writes a SimConfig back to a JSON file  
void saveConfigTemplate(const std::string& path);

 
void printConfig(const SimConfig& cfg);
