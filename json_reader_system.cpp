#include "json.hpp"
#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <optional>

using namespace std;
using json = nlohmann::json;

// Helper function to check if a JSON value is effectively empty
bool isEffectivelyEmpty(const json& j) {
    if (j.is_null()) return true;
    if (j.is_array() && j.empty()) return true;
    if (j.is_object() && j.empty()) return true;
    if (j.is_string() && j.get<string>().empty()) return true;
    return false;
}

// Helper function to add non-empty fields to output
void addIfNotEmpty(json& output, const string& key, const json& value) {
    if (!isEffectivelyEmpty(value)) {
        output[key] = value;
    }
}

// Structure for section mappings from structure.json
struct SectionMapping {
    string sectionName;
    string group;
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
    
    map<string, SectionMapping> sectionMappings;
    
    // Internal AI scoring data (not exported)
    map<string, int> layeringRoles;  // 1-6 layering stages (internal only)
    map<string, float> aiScores;     // AI matching scores (internal only)

public:
    // Load all JSON files
    bool loadJsonFiles() {
        try {
            // Load actual instrument/group data
            ifstream guitarFile("guitar.json");
            if (!guitarFile) {
                cerr << "Error: Cannot open guitar.json" << endl;
                return false;
            }
            guitarFile >> guitarData;
            
            ifstream groupFile("group.json");
            if (!groupFile) {
                cerr << "Error: Cannot open group.json" << endl;
                return false;
            }
            groupFile >> groupData;
            
            // Load reference data (for AI scoring only)
            ifstream moodsFile("moods.json");
            if (moodsFile) {
                moodsFile >> moodsData;
            }
            
            ifstream synthFile("Synthesizer.json");
            if (synthFile) {
                synthFile >> synthData;
            }
            
            // Load structure mapping
            ifstream structureFile("structure.json");
            if (structureFile) {
                structureFile >> structureData;
                loadSectionMappings();
            }
            
            return true;
        } catch (const exception& e) {
            cerr << "Error loading JSON files: " << e.what() << endl;
            return false;
        }
    }
    
private:
    // Load section mappings from structure.json
    void loadSectionMappings() {
        if (!structureData.contains("sections") || !structureData["sections"].is_array()) {
            return;
        }
        
        for (const auto& section : structureData["sections"]) {
            if (!section.is_object() || !section.contains("group") || !section.contains("sectionName")) {
                continue;
            }
            
            SectionMapping mapping;
            mapping.sectionName = section["sectionName"].get<string>();
            mapping.group = section["group"].get<string>();
            
            if (section.contains("attackMul")) mapping.attackMul = section["attackMul"].get<float>();
            if (section.contains("decayMul")) mapping.decayMul = section["decayMul"].get<float>();
            if (section.contains("sustainMul")) mapping.sustainMul = section["sustainMul"].get<float>();
            if (section.contains("releaseMul")) mapping.releaseMul = section["releaseMul"].get<float>();
            if (section.contains("useDynamicGate")) mapping.useDynamicGate = section["useDynamicGate"].get<bool>();
            if (section.contains("gateThreshold")) mapping.gateThreshold = section["gateThreshold"].get<float>();
            if (section.contains("gateDecaySec")) mapping.gateDecaySec = section["gateDecaySec"].get<float>();
            
            sectionMappings[mapping.group] = mapping;
        }
    }
    
    // Process guitar instruments from guitar.json
    json processGuitarInstruments() {
        json output = json::object();
        
        if (!guitarData.contains("guitar_types") || !guitarData["guitar_types"].is_object()) {
            return output;
        }
        
        for (const auto& [guitarType, typeData] : guitarData["guitar_types"].items()) {
            if (!typeData.contains("groups") || !typeData["groups"].is_object()) {
                continue;
            }
            
            for (const auto& [instrumentName, instrumentData] : typeData["groups"].items()) {
                json cleanInstrument = processGuitarInstrument(instrumentData, instrumentName);
                if (!cleanInstrument.empty()) {
                    output[instrumentName] = cleanInstrument;
                }
            }
        }
        
        // Process articulations as well
        if (guitarData.contains("articulations") && guitarData["articulations"].contains("groups")) {
            for (const auto& [articulationName, articulationData] : guitarData["articulations"]["groups"].items()) {
                json cleanArticulation = processGuitarInstrument(articulationData, articulationName);
                if (!cleanArticulation.empty()) {
                    output[articulationName] = cleanArticulation;
                }
            }
        }
        
        return output;
    }
    
    // Process a single guitar instrument, flattening and cleaning
    json processGuitarInstrument(const json& instrumentData, const string& instrumentName) {
        json result = json::object();
        
        // Process ADSR/Envelope
        if (instrumentData.contains("envelope") && instrumentData["envelope"].is_object()) {
            json adsr = json::object();
            const auto& envelope = instrumentData["envelope"];
            
            if (envelope.contains("type")) adsr["type"] = envelope["type"];
            if (envelope.contains("attack")) adsr["attack"] = envelope["attack"];
            if (envelope.contains("decay")) adsr["decay"] = envelope["decay"];
            if (envelope.contains("sustain")) adsr["sustain"] = envelope["sustain"];
            if (envelope.contains("release")) adsr["release"] = envelope["release"];
            if (envelope.contains("hold")) adsr["hold"] = envelope["hold"];
            if (envelope.contains("delay")) adsr["delay"] = envelope["delay"];
            if (envelope.contains("curve")) adsr["curve"] = envelope["curve"];
            
            addIfNotEmpty(result, "adsr", adsr);
        }
        
        // Process guitar-specific parameters
        json guitarParams = json::object();
        
        // Strings
        if (instrumentData.contains("strings") && instrumentData["strings"].is_object()) {
            addIfNotEmpty(guitarParams, "strings", instrumentData["strings"]);
        }
        
        // Harmonics
        if (instrumentData.contains("harmonics") && instrumentData["harmonics"].is_object()) {
            addIfNotEmpty(guitarParams, "harmonics", instrumentData["harmonics"]);
        }
        
        // Filter
        if (instrumentData.contains("filter") && instrumentData["filter"].is_object()) {
            addIfNotEmpty(guitarParams, "filter", instrumentData["filter"]);
        }
        
        // Attack noise
        if (instrumentData.contains("attack_noise") && instrumentData["attack_noise"].is_object()) {
            addIfNotEmpty(guitarParams, "attackNoise", instrumentData["attack_noise"]);
        }
        
        // Body resonance
        if (instrumentData.contains("body_resonance") && instrumentData["body_resonance"].is_object()) {
            addIfNotEmpty(guitarParams, "bodyResonance", instrumentData["body_resonance"]);
        }
        
        // Pick
        if (instrumentData.contains("pick") && instrumentData["pick"].is_object()) {
            addIfNotEmpty(guitarParams, "pick", instrumentData["pick"]);
        }
        
        // Vibrato
        if (instrumentData.contains("vibrato") && instrumentData["vibrato"].is_object()) {
            addIfNotEmpty(guitarParams, "vibrato", instrumentData["vibrato"]);
        }
        
        addIfNotEmpty(result, "guitarParams", guitarParams);
        
        // Effects
        if (instrumentData.contains("fx") && instrumentData["fx"].is_array() && !instrumentData["fx"].empty()) {
            result["effects"] = instrumentData["fx"];
        }
        
        // Sound characteristics
        if (instrumentData.contains("sound_characteristics") && instrumentData["sound_characteristics"].is_object()) {
            addIfNotEmpty(result, "soundCharacteristics", instrumentData["sound_characteristics"]);
        }
        
        // Topological metadata
        if (instrumentData.contains("topological_metadata") && instrumentData["topological_metadata"].is_object()) {
            addIfNotEmpty(result, "topologicalMetadata", instrumentData["topological_metadata"]);
        }
        
        // Apply structure mapping if it exists
        if (sectionMappings.find(instrumentName) != sectionMappings.end()) {
            json structure = createStructureMapping(sectionMappings[instrumentName]);
            addIfNotEmpty(result, "structure", structure);
        }
        
        return result;
    }
    
    // Process group effects from group.json
    json processGroupEffects() {
        json output = json::object();
        
        if (!groupData.contains("groups") || !groupData["groups"].is_object()) {
            return output;
        }
        
        for (const auto& [groupName, groupData] : groupData["groups"].items()) {
            json cleanGroup = processGroupEffect(groupData, groupName);
            if (!cleanGroup.empty()) {
                output[groupName] = cleanGroup;
            }
        }
        
        return output;
    }
    
    // Process a single group effect, flattening and cleaning
    json processGroupEffect(const json& groupData, const string& groupName) {
        json result = json::object();
        
        // Synthesis type
        if (groupData.contains("synthesis_type")) {
            result["synthesisType"] = groupData["synthesis_type"];
        }
        
        // Oscillator
        if (groupData.contains("oscillator") && groupData["oscillator"].is_object()) {
            addIfNotEmpty(result, "oscillator", groupData["oscillator"]);
        }
        
        // Envelope (ADSR)
        if (groupData.contains("envelope") && groupData["envelope"].is_object()) {
            addIfNotEmpty(result, "adsr", groupData["envelope"]);
        }
        
        // Filter
        if (groupData.contains("filter") && groupData["filter"].is_object()) {
            addIfNotEmpty(result, "filter", groupData["filter"]);
        }
        
        // Effects
        if (groupData.contains("fx") && groupData["fx"].is_array() && !groupData["fx"].empty()) {
            result["effects"] = groupData["fx"];
        }
        
        // Sound characteristics
        if (groupData.contains("sound_characteristics") && groupData["sound_characteristics"].is_object()) {
            addIfNotEmpty(result, "soundCharacteristics", groupData["sound_characteristics"]);
        }
        
        // Topological metadata
        if (groupData.contains("topological_metadata") && groupData["topological_metadata"].is_object()) {
            addIfNotEmpty(result, "topologicalMetadata", groupData["topological_metadata"]);
        }
        
        // Apply structure mapping if it exists
        if (sectionMappings.find(groupName) != sectionMappings.end()) {
            json structure = createStructureMapping(sectionMappings[groupName]);
            addIfNotEmpty(result, "structure", structure);
        }
        
        return result;
    }
    
    // Create structure mapping JSON from SectionMapping
    json createStructureMapping(const SectionMapping& mapping) {
        json structure = json::object();
        
        structure["sectionName"] = mapping.sectionName;
        
        if (mapping.attackMul != 1.0f) structure["attackMul"] = mapping.attackMul;
        if (mapping.decayMul != 1.0f) structure["decayMul"] = mapping.decayMul;
        if (mapping.sustainMul != 1.0f) structure["sustainMul"] = mapping.sustainMul;
        if (mapping.releaseMul != 1.0f) structure["releaseMul"] = mapping.releaseMul;
        
        if (mapping.useDynamicGate) {
            structure["useDynamicGate"] = mapping.useDynamicGate;
            structure["gateThreshold"] = mapping.gateThreshold;
            structure["gateDecaySec"] = mapping.gateDecaySec;
        }
        
        return structure;
    }
    
    // Internal AI scoring functions (used internally but not exported)
    void calculateAIScores() {
        // This is where AI scoring based on moods.json and Synthesizer.json would happen
        // Results stored in aiScores map but never exported to config
        cout << "Calculating AI scores using reference data..." << endl;
        
        // Example internal scoring logic
        if (moodsData.contains("moods")) {
            cout << "Processing mood reference data for scoring..." << endl;
        }
        
        if (synthData.contains("sections")) {
            cout << "Processing synthesizer reference data for scoring..." << endl;
        }
    }
    
    void calculateLayeringRoles() {
        // This calculates 1-6 layering stages but keeps them internal
        cout << "Calculating layering roles (1-6 stages) for internal use..." << endl;
        
        // Example layering calculation (internal only)
        layeringRoles["Pad_Warm_Calm"] = 1;  // Background
        layeringRoles["Bass_Punchy_Driving"] = 2;  // Foundation
        layeringRoles["Chord_Soft_Lush"] = 3;  // Harmony
        layeringRoles["Lead_Bright_Energetic"] = 5;  // Lead
        layeringRoles["Bell_Glassy_Clear"] = 6;  // Foreground details
    }

public:
    // Generate the final clean configuration
    json generateCleanConfig() {
        json finalConfig = json::object();
        
        // Process guitar instruments and add them at top level
        json guitarInstruments = processGuitarInstruments();
        for (const auto& [name, config] : guitarInstruments.items()) {
            finalConfig[name] = config;
        }
        
        // Process group effects and add them at top level
        json groupEffects = processGroupEffects();
        for (const auto& [name, config] : groupEffects.items()) {
            finalConfig[name] = config;
        }
        
        // Calculate internal AI data (but don't export it)
        calculateAIScores();
        calculateLayeringRoles();
        
        return finalConfig;
    }
    
    // Save configuration to file
    bool saveConfig(const string& filename) {
        try {
            json config = generateCleanConfig();
            
            ofstream outFile(filename);
            if (!outFile) {
                cerr << "Error: Cannot create output file " << filename << endl;
                return false;
            }
            
            outFile << config.dump(2);
            outFile.close();
            
            cout << "Clean configuration saved to " << filename << endl;
            cout << "Total instruments/groups processed: " << config.size() << endl;
            
            return true;
        } catch (const exception& e) {
            cerr << "Error saving configuration: " << e.what() << endl;
            return false;
        }
    }
    
    // Print summary of what was processed
    void printSummary() {
        cout << "\n=== JSON Reader System Summary ===" << endl;
        cout << "Reference files loaded for AI scoring:" << endl;
        cout << "  - moods.json: " << (moodsData.empty() ? "Not loaded" : "Loaded") << endl;
        cout << "  - Synthesizer.json: " << (synthData.empty() ? "Not loaded" : "Loaded") << endl;
        
        cout << "\nSource files processed for config output:" << endl;
        cout << "  - guitar.json instruments/articulations processed" << endl;
        cout << "  - group.json effects processed" << endl;
        
        cout << "\nSection mappings loaded: " << sectionMappings.size() << endl;
        
        cout << "\nInternal AI data calculated (not exported):" << endl;
        cout << "  - AI scores: " << aiScores.size() << " items" << endl;
        cout << "  - Layering roles: " << layeringRoles.size() << " items" << endl;
        cout << "=================================" << endl;
    }
};

int main() {
    cout << "JSON Reader System - Clean Configuration Generator" << endl;
    cout << "=================================================" << endl;
    
    JsonReaderSystem system;
    
    // Load all JSON files
    if (!system.loadJsonFiles()) {
        cerr << "Failed to load JSON files. Exiting." << endl;
        return 1;
    }
    
    // Generate and save clean configuration
    if (!system.saveConfig("clean_config.json")) {
        cerr << "Failed to save configuration. Exiting." << endl;
        return 1;
    }
    
    // Print summary
    system.printSummary();
    
    cout << "\nConfiguration generated successfully!" << endl;
    cout << "Next stage: WAV writer will use this clean config for synthesis." << endl;
    
    return 0;
}