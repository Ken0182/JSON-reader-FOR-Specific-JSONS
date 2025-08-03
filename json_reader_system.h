#ifndef JSON_READER_SYSTEM_H
#define JSON_READER_SYSTEM_H

#include "json.hpp"
#include "ParsedId.h"
#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <optional>
#include <cmath>
#include <numeric>
#include <cassert>

using json = nlohmann::json;

// Structure for section mappings from structure.json
struct SectionMapping {
    std::string sectionName;
    std::string group;
    float attackMul = 1.0f;
    float decayMul = 1.0f;
    float sustainMul = 1.0f;
    float releaseMul = 1.0f;
    bool useDynamicGate = false;
    float gateThreshold = 0.0f;
    float gateDecaySec = 0.0f;
};

// Main JSON Reader System class
class JsonReaderSystem {
private:
    json guitarData;
    json groupData;
    json moodsData;      // Reference only - for AI scoring
    json synthData;      // Reference only - for AI scoring
    json structureData;
    
    std::map<std::string, SectionMapping> sectionMappings;
    
    // Internal AI scoring data (not exported)
    std::map<std::string, int> layeringRoles;  // 1-6 layering stages (internal only)
    std::map<std::string, float> aiScores;     // AI matching scores (internal only)
    
    // Category averages for inference
    std::map<std::string, std::map<std::string, float>> categoryAverages;

    // 4Z ID Generation Functions
    int determineDim(const json& entry, const std::string& entryType);
    float extractTransientIntensity(const json& entry, const std::string& category = "instrument");
    int extractHarmonicComplexity(const json& entry, const std::string& category = "instrument");
    int extractFxComplexity(const json& entry);
    int extractTuningPrime(const json& entry);
    int extractDynamicRange(const json& entry);
    int extractFrequencyRange(const json& entry);
    std::string formatAttributes(int trans, int harm, int fx, int tuning, int damp, int freq);
    int quantizeTransients(float intensity);
    std::string generateId(const json& entry, const std::string& entryType);
    std::string determineCategory(const json& entry);

    // Internal processing functions
    void loadSectionMappings();
    json processGuitarInstruments();
    json processGuitarInstrument(const json& instrumentData, const std::string& instrumentName);
    json processGroupEffects();
    json processGroupEffect(const json& groupData, const std::string& groupName);
    json createStructureMapping(const SectionMapping& mapping);
    void calculateAIScores();
    void calculateLayeringRoles();

public:
    JsonReaderSystem();
    
    // Public interface
    bool loadJsonFiles(const std::string& basePath = ".");
    void generateAllIds();
    ParsedId parseId(const std::string& id);
    json generateCleanConfig();
    void testWithMocks();
    bool saveConfig(const std::string& filename);
    void printSummary();
};

// Helper functions
bool isEffectivelyEmpty(const json& j);
void addIfNotEmpty(json& output, const std::string& key, const json& value);

#endif // JSON_READER_SYSTEM_H