#include "config_loader.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <iomanip>

// ─────────────────────────────────────────────
//  Minimal JSON parser helpers
// ─────────────────────────────────────────────
namespace {

struct JsonParser {
    const std::string& src;
    size_t pos = 0;

    void skipWS() {
        while (pos < src.size() && std::isspace((unsigned char)src[pos])) ++pos;
    }
    char peek() { skipWS(); return pos < src.size() ? src[pos] : '\0'; }
    char get()  { skipWS(); return pos < src.size() ? src[pos++] : '\0'; }

    void expect(char c) {
        if (get() != c) throw std::runtime_error(
            std::string("JSON parse error: expected '") + c + "' at pos " + std::to_string(pos));
    }

    std::string parseString() {
        expect('"');
        std::string s;
        while (pos < src.size()) {
            char c = src[pos++];
            if (c == '"') break;
            if (c == '\\' && pos < src.size()) { s += src[pos++]; }
            else s += c;
        }
        return s;
    }

    double parseNumber() {
        skipWS();
        size_t start = pos;
        if (pos < src.size() && (src[pos] == '-' || src[pos] == '+')) ++pos;
        while (pos < src.size() && (std::isdigit((unsigned char)src[pos]) ||
               src[pos] == '.' || src[pos] == 'e' || src[pos] == 'E' ||
               src[pos] == '+' || src[pos] == '-')) ++pos;
        return std::stod(src.substr(start, pos - start));
    }

    std::vector<std::string> parseStringArray() {
        expect('[');
        std::vector<std::string> v;
        while (peek() != ']') {
            v.push_back(parseString());
            if (peek() == ',') get();
        }
        get(); // ']'
        return v;
    }

    // Returns true if key matches, advances past ':'
    bool tryKey(const std::string& key) {
        size_t save = pos;
        skipWS();
        if (peek() != '"') { pos = save; return false; }
        std::string k = parseString();
        if (k != key) { pos = save; return false; }
        expect(':');
        return true;
    }
};

MachineCategory parseMachineCategory(JsonParser& p) {
    p.expect('{');
    MachineCategory cat;
    while (p.peek() != '}') {
        std::string key = p.parseString();
        p.expect(':');
        if      (key == "name")  cat.name  = p.parseString();
        else if (key == "count") cat.count = static_cast<int>(p.parseNumber());
        else if (key == "mttf")  cat.mttf  = p.parseNumber();
        else if (key == "mttr")  cat.mttR  = p.parseNumber();
        if (p.peek() == ',') p.get();
    }
    p.get(); // '}'
    return cat;
}

AdjusterProfile parseAdjusterProfile(JsonParser& p) {
    p.expect('{');
    AdjusterProfile prof;
    while (p.peek() != '}') {
        std::string key = p.parseString();
        p.expect(':');
        if      (key == "name")      prof.name      = p.parseString();
        else if (key == "expertise") prof.expertise = p.parseStringArray();
        if (p.peek() == ',') p.get();
    }
    p.get();
    return prof;
}

} // anonymous namespace

// ─────────────────────────────────────────────
//  Public API
// ─────────────────────────────────────────────
SimConfig loadConfig(const std::string& path)
{
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Cannot open config file: " + path);
    std::stringstream ss; ss << f.rdbuf();
    std::string src = ss.str();

    JsonParser p{src};
    p.expect('{');

    SimConfig cfg;
    cfg.randomSeed   = 42;
    cfg.simDuration  = 0;

    while (p.peek() != '}') {
        std::string key = p.parseString();
        p.expect(':');

        if (key == "sim_duration") {
            cfg.simDuration = p.parseNumber();
        } else if (key == "random_seed") {
            cfg.randomSeed = static_cast<unsigned int>(p.parseNumber());
        } else if (key == "machine_categories") {
            p.expect('[');
            while (p.peek() != ']') {
                cfg.categories.push_back(parseMachineCategory(p));
                if (p.peek() == ',') p.get();
            }
            p.get();
        } else if (key == "adjuster_types") {
            p.expect('[');
            while (p.peek() != ']') {
                cfg.adjusterTypes.push_back(parseAdjusterProfile(p));
                if (p.peek() == ',') p.get();
            }
            p.get();
        } else if (key == "adjuster_counts") {
            p.expect('[');
            while (p.peek() != ']') {
                cfg.adjusterCounts.push_back(static_cast<int>(p.parseNumber()));
                if (p.peek() == ',') p.get();
            }
            p.get();
        }

        if (p.peek() == ',') p.get();
    }

    if (cfg.simDuration <= 0)
        throw std::runtime_error("sim_duration must be positive.");
    if (cfg.categories.empty())
        throw std::runtime_error("At least one machine_category is required.");
    if (cfg.adjusterTypes.empty())
        throw std::runtime_error("At least one adjuster_type is required.");
    if (cfg.adjusterCounts.size() != cfg.adjusterTypes.size())
        throw std::runtime_error("adjuster_counts must have the same length as adjuster_types.");

    return cfg;
}

void saveConfigTemplate(const std::string& path)
{
    std::ofstream f(path);
    if (!f) throw std::runtime_error("Cannot write config template to: " + path);

    f << R"({
  "sim_duration": 2000,
  "random_seed": 42,
  "machine_categories": [
    { "name": "Lathe",     "count": 200, "mttf": 12.0, "mttr": 2.0 },
    { "name": "Turning",   "count": 50,  "mttf": 15.0, "mttr": 1.5 },
    { "name": "Drilling",  "count": 80,  "mttf": 10.0, "mttr": 1.0 },
    { "name": "Soldering", "count": 30,  "mttf": 20.0, "mttr": 0.5 }
  ],
  "adjuster_types": [
    {
      "name": "GeneralAdjuster",
      "expertise": ["Lathe", "Drilling"]
    },
    {
      "name": "SpecialistAdjuster",
      "expertise": ["Turning", "Soldering"]
    }
  ],
  "adjuster_counts": [10, 5]
}
)";
}

void printConfig(const SimConfig& cfg)
{
    std::cout << "═══════════════════════════════════════════════\n";
    std::cout << "  FSSS  —  Simulation Configuration\n";
    std::cout << "═══════════════════════════════════════════════\n";
    std::cout << "  Sim duration : " << cfg.simDuration << " h\n";
    std::cout << "  Random seed  : " << cfg.randomSeed  << "\n\n";

    std::cout << "  Machine Categories:\n";
    std::cout << "  " << std::left
              << std::setw(15) << "Name"
              << std::setw(8)  << "Count"
              << std::setw(10) << "MTTF(h)"
              << std::setw(10) << "MTTR(h)" << "\n";
    std::cout << "  " << std::string(43, '-') << "\n";
    for (const auto& c : cfg.categories)
        std::cout << "  " << std::left
                  << std::setw(15) << c.name
                  << std::setw(8)  << c.count
                  << std::setw(10) << c.mttf
                  << std::setw(10) << c.mttR << "\n";

    std::cout << "\n  Adjuster Types:\n";
    for (size_t i = 0; i < cfg.adjusterTypes.size(); ++i) {
        const auto& t = cfg.adjusterTypes[i];
        int cnt = (i < cfg.adjusterCounts.size()) ? cfg.adjusterCounts[i] : 0;
        std::cout << "  " << t.name << " × " << cnt << " — expertise: [";
        for (size_t j = 0; j < t.expertise.size(); ++j) {
            if (j) std::cout << ", ";
            std::cout << t.expertise[j];
        }
        std::cout << "]\n";
    }
    std::cout << "═══════════════════════════════════════════════\n\n";
}
