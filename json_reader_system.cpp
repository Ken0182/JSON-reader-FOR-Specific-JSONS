#include "json_reader_system.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <numeric>

using json = nlohmann::json;

// Helper function to check if a JSON value is effectively empty
bool isEffectivelyEmpty(const json& j) {
    if (j.is_null()) return true;
    if (j.is_array() && j.empty()) return true;
    if (j.is_object() && j.empty()) return true;
    if (j.is_string() && j.get<std::string>().empty()) return true;
    return false;
}

// Helper function to add non-empty fields to output
void addIfNotEmpty(json& output, const std::string& key, const json& value) {
    if (!isEffectivelyEmpty(value)) {
        output[key] = value;
    }
}

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

// 4Z ID Structure
struct ParsedId {
    int dim;
    int trans_digit;
    int harm_digit;
    int fx_digit;
    int tuning_prime;
    int damp_digit;
    int freq_digit;
    char type;
};

// Main JSON Reader System class
class JsonReaderSystem {
private:
    json guitarData;
    json groupData;
    json moodsData;      // Reference only - for AI scoring
    json synthData;      // Reference only - for AI scoring
    json structureData;
    
    std::unordered_map<std::string, SectionMapping> sectionMappings;
    
    // Internal AI scoring data (not exported)
    std::unordered_map<std::string, int> layeringRoles;  // 1-6 layering stages (internal only)
    std::unordered_map<std::string, float> aiScores;     // AI matching scores (internal only)
    
    // Category averages for inference
    std::unordered_map<std::string, std::unordered_map<std::string, float>> categoryAverages = {
        {"pad", {
            {"harmonicRichness", 0.5f},
            {"transientSharpness", 0.3f},
            {"fxComplexity", 0.4f},
            {"frequencyFocus", 0.5f},
            {"dynamicCompression", 0.7f}
        }},
        {"lead", {
            {"harmonicRichness", 0.7f},
            {"transientSharpness", 0.8f},
            {"fxComplexity", 0.6f},
            {"frequencyFocus", 0.8f},
            {"dynamicCompression", 0.4f}
        }},
        {"bass", {
            {"harmonicRichness", 0.4f},
            {"transientSharpness", 0.7f},
            {"fxComplexity", 0.3f},
            {"frequencyFocus", 0.2f},
            {"dynamicCompression", 0.6f}
        }},
        {"guitar", {
            {"harmonicRichness", 0.6f},
            {"transientSharpness", 0.5f},
            {"fxComplexity", 0.4f},
            {"frequencyFocus", 0.6f},
            {"dynamicCompression", 0.5f}
        }}
    };

    // 4Z ID Generation Functions
    int determineDim(const json& entry, const std::string& entryType) {
        // 1 = semantic-rich (many tags/emotional content)
        if (entry.contains("emotional") || entry.contains("tags")) {
            auto emotional = entry.value("emotional", json::array());
            if (emotional.size() > 3) return 1;
        }
        
        // 2 = technical (envelope/synthesis focused)
        if (entry.contains("envelope") || entry.contains("oscillator") || 
            entry.contains("synthesis_type")) return 2;
        
        // 4 = layering (frequency/dynamic focused)
        if (entry.contains("frequencyRange") || entry.contains("dynamicRange") ||
            entry.contains("texture_density")) return 4;
        
        // 3 = musical role (default)
        return 3;
    }
    
    float extractTransientIntensity(const json& entry) {
        if (entry.contains("transientDetail") && entry["transientDetail"].contains("intensity")) {
            auto intensity = entry["transientDetail"]["intensity"];
            if (intensity.is_array() && intensity.size() >= 2) {
                return (intensity[0].get<float>() + intensity[1].get<float>()) / 2.0f;
            }
        }
        
        // Fallback: infer from attack_noise or envelope attack
        if (entry.contains("attack_noise") && entry["attack_noise"].contains("intensity")) {
            auto intensity = entry["attack_noise"]["intensity"];
            if (intensity.is_array() && intensity.size() >= 2) {
                return (intensity[0].get<float>() + intensity[1].get<float>()) / 2.0f;
            }
        }
        
        if (entry.contains("envelope") && entry["envelope"].contains("attack")) {
            auto attack = entry["envelope"]["attack"];
            if (attack.is_array() && attack.size() >= 2) {
                float avg_attack = (attack[0].get<float>() + attack[1].get<float>()) / 2.0f;
                // Research-based log scaling: psychoacoustics masking implies log for perception
                // log10(attack_ms + 1) / log10(10000) for 0-1 sharp, then inverse for sharpness
                return 1.0f - (std::log10(avg_attack * 1000 + 1) / std::log10(10000));
            }
        }
        
        return 0.5f; // NA default
    }
    
    int extractHarmonicComplexity(const json& entry, const std::string& category = "unknown") {
        if (entry.contains("harmonicContent")) {
            auto complexity = entry["harmonicContent"].value("complexity", "unknown");
            if (complexity == "low") return 25;
            else if (complexity == "medium" || complexity == "med") return 50;
            else if (complexity == "high") return 75;
            else if (complexity == "very high") return 90;
            else if (complexity == "extreme") return 99;
            
            // Infer from overtones array length (K-means style)
            if (entry["harmonicContent"].contains("overtones")) {
                auto overtones = entry["harmonicContent"]["overtones"];
                if (overtones.is_array()) {
                    int len = overtones.size();
                    if (len < 3) {
                        // Use category average for partial data
                        if (categoryAverages.count(category) && categoryAverages[category].count("harmonicRichness")) {
                            return static_cast<int>(categoryAverages[category]["harmonicRichness"] * 99);
                        }
                        return 25; // low
                    }
                    else if (len <= 6) return 50; // medium
                    else return 75; // high
                }
            }
        }
        
        // Fallback: infer from vibe_set or harmonics
        if (entry.contains("harmonics") && entry["harmonics"].contains("vibe_set")) {
            auto vibe_set = entry["harmonics"]["vibe_set"];
            if (vibe_set.is_array()) {
                int len = vibe_set.size();
                if (len < 3) {
                    if (categoryAverages.count(category) && categoryAverages[category].count("harmonicRichness")) {
                        return static_cast<int>(categoryAverages[category]["harmonicRichness"] * 99);
                    }
                    return 25;
                }
                else if (len <= 5) return 50;
                else return 75;
            }
        }
        
        // Use category average if available
        if (categoryAverages.count(category) && categoryAverages[category].count("harmonicRichness")) {
            return static_cast<int>(categoryAverages[category]["harmonicRichness"] * 99);
        }
        
        return 50; // NA default (medium)
    }
    
    int extractFxComplexity(const json& entry) {
        int fx_count = 0;
        
        if (entry.contains("fxCategories") && entry["fxCategories"].is_array()) {
            fx_count = entry["fxCategories"].size();
        } else if (entry.contains("fx")) {
            // Count enabled effects
            if (entry["fx"].is_object()) {
                for (auto& [key, value] : entry["fx"].items()) {
                    if (value.is_object() && value.value("enabled", false)) {
                        fx_count++;
                    }
                }
            } else if (entry["fx"].is_array()) {
                fx_count = entry["fx"].size();
            }
        }
        
        // Quantize: 0=00, 1-2=20, 3-4=50, 5+=80, cap at 99
        if (fx_count == 0) return 0;
        else if (fx_count <= 2) return 20;
        else if (fx_count <= 4) return 50;
        else return std::min(fx_count * 16, 99);
    }
    
    int extractTuningPrime(const json& entry) {
        std::string tuning = entry.value("theoryTuning", "unknown");
        if (tuning == "equal") return 2;
        else if (tuning == "microtonal" || tuning == "micro") return 3;
        else if (tuning == "just" || tuning == "just_intonation") return 5;
        else if (tuning == "atonal") return 11;
        
        // Infer from detune if available
        if (entry.contains("oscillator") && entry["oscillator"].contains("detune")) {
            auto detune = entry["oscillator"]["detune"];
            float detune_val = 0.0f;
            if (detune.is_array() && detune.size() >= 2) {
                detune_val = (detune[0].get<float>() + detune[1].get<float>()) / 2.0f;
            } else if (detune.is_number()) {
                detune_val = detune.get<float>();
            }
            
            if (std::abs(detune_val) > 0.05f) return 3; // microtonal
        }
        
        return 7; // NA default
    }
    
    int extractDynamicRange(const json& entry) {
        std::string range = entry.value("dynamicRange", "unknown");
        
        if (range.find("compressed") != std::string::npos || 
            range.find("high damping") != std::string::npos ||
            range == "highly compressed") return 20;
        else if (range.find("moderate") != std::string::npos ||
                 range.find("medium") != std::string::npos) return 50;
        else if (range.find("expanded") != std::string::npos ||
                 range.find("low damping") != std::string::npos) return 80;
        else if (range == "maximum" || range == "chaotic") return 99;
        
        // Infer from damping or release time
        if (entry.contains("topological_metadata") && 
            entry["topological_metadata"].contains("damping")) {
            std::string damping = entry["topological_metadata"]["damping"];
            if (damping == "high" || damping == "very_high") return 20;
            else if (damping == "moderate") return 50;
            else if (damping == "low") return 80;
        }
        
        if (entry.contains("envelope") && entry["envelope"].contains("release")) {
            auto release = entry["envelope"]["release"];
            if (release.is_array() && release.size() >= 2) {
                float avg_release = (release[0].get<float>() + release[1].get<float>()) / 2.0f;
                // Exponential mapping: short release = compressed
                return static_cast<int>(std::min(99.0f, std::exp(-avg_release/1000.0f) * 99.0f));
            }
        }
        
        return 50; // NA default
    }
    
    int extractFrequencyRange(const json& entry) {
        std::string range = entry.value("frequencyRange", "unknown");
        
        if (range == "low" || range == "low-mid") return 25;
        else if (range == "mid" || range == "mid-high") return 50;
        else if (range == "high" || range == "high-focused") return 75;
        else if (range == "full-spectrum" || range == "full") return 99;
        else if (range == "limited") return 15;
        
        // Infer from filter cutoff or instrument type
        if (entry.contains("filter") && entry["filter"].contains("cutoff")) {
            auto cutoff = entry["filter"]["cutoff"];
            if (cutoff.is_array() && cutoff.size() >= 2) {
                float avg_cutoff = (cutoff[0].get<float>() + cutoff[1].get<float>()) / 2.0f;
                if (avg_cutoff < 500) return 25; // low
                else if (avg_cutoff < 2000) return 50; // mid
                else return 75; // high
            }
        }
        
        // Infer from instrument type
        std::string type = entry.value("type", "");
        if (type.find("bass") != std::string::npos || 
            type.find("Bass") != std::string::npos) return 25;
        
        return 50; // NA default (mid)
    }
    
    std::string formatAttributes(int trans, int harm, int fx, int tuning, int damp, int freq) {
        char buffer[13];
        snprintf(buffer, 13, "%02d%02d%02d%d%02d%02d", trans, harm, fx, tuning, damp, freq);
        return std::string(buffer);
    }
    
    int quantize(float val, float min, float max, int bins) {
        if (val < min || val > max) return bins / 2;
        return static_cast<int>((val - min) / (max - min) * bins);
    }
    
    int gcd(int a, int b) {
        while (b != 0) {
            int temp = b;
            b = a % b;
            a = temp;
        }
        return a;
    }
    
    std::string generateId(const json& entry, const std::string& entryType) {
        int dim = determineDim(entry, entryType);
        
        // Extract and quantize properties with fallbacks
        float trans_avg = extractTransientIntensity(entry);
        int trans_digit = quantize(trans_avg, 0.0f, 1.0f, 99);
        
        std::string category = determineCategory(entry);
        int harm_digit = extractHarmonicComplexity(entry, category);
        int fx_digit = extractFxComplexity(entry);
        int tuning_prime = extractTuningPrime(entry);
        int damp_digit = extractDynamicRange(entry);
        int freq_digit = extractFrequencyRange(entry);
        
        // Concatenate attributes
        std::string attrs = formatAttributes(trans_digit, harm_digit, fx_digit, 
                                       tuning_prime, damp_digit, freq_digit);
        
        char type = entryType[0]; // i/g/x/m/s
        return std::to_string(dim) + "." + attrs + type;
    }
    
    std::string determineCategory(const json& entry) {
        if (entry.contains("guitarParams")) return "guitar";
        if (entry.contains("synthesisType")) return "group";
        
        // Check name patterns
        for (auto& [key, value] : entry.items()) {
            std::string keyLower = key;
            std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(), ::tolower);
            if (keyLower.find("bass") != std::string::npos) return "bass";
            if (keyLower.find("lead") != std::string::npos) return "lead";
            if (keyLower.find("pad") != std::string::npos) return "pad";
        }
        
        return "instrument";
    }

public:
    // Load all JSON files
    bool loadJsonFiles(const std::string& basePath = ".") {
        try {
            // Load actual instrument/group data
            std::ifstream guitarFile(basePath + "/guitar.json");
            if (!guitarFile) {
                std::cerr << "Error: Cannot open guitar.json" << std::endl;
                return false;
            }
            guitarFile >> guitarData;
            
            std::ifstream groupFile(basePath + "/group.json");
            if (!groupFile) {
                std::cerr << "Error: Cannot open group.json" << std::endl;
                return false;
            }
            groupFile >> groupData;
            
            // Load reference data (for AI scoring only)
            std::ifstream moodsFile(basePath + "/moods.json");
            if (moodsFile) {
                moodsFile >> moodsData;
            }
            
            std::ifstream synthFile(basePath + "/Synthesizer.json");
            if (synthFile) {
                synthFile >> synthData;
            }
            
            // Load structure mapping
            std::ifstream structureFile(basePath + "/structure.json");
            if (structureFile) {
                structureFile >> structureData;
                loadSectionMappings();
            }
            
            // Generate IDs for all entries
            generateAllIds();
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error loading JSON files: " << e.what() << std::endl;
            return false;
        }
    }
    
    void generateAllIds() {
        // Generate IDs for guitar entries
        if (guitarData.contains("guitar_types")) {
            for (auto& [guitarType, typeData] : guitarData["guitar_types"].items()) {
                if (typeData.contains("groups")) {
                    for (auto& [key, group] : typeData["groups"].items()) {
                        group["id"] = generateId(group, "instrument");
                    }
                }
            }
        }
        if (guitarData.contains("articulations") && guitarData["articulations"].contains("groups")) {
            for (auto& [key, articulation] : guitarData["articulations"]["groups"].items()) {
                articulation["id"] = generateId(articulation, "instrument");
            }
        }
        
        // Generate IDs for group entries
        if (groupData.contains("groups")) {
            for (auto& [key, group] : groupData["groups"].items()) {
                group["id"] = generateId(group, "group");
            }
        }
        
        // Generate IDs for other data sources as needed
        std::cout << "Generated IDs for all entries" << std::endl;
    }
    
    ParsedId parseId(const std::string& id) {
        ParsedId parsed;
        
        // Split by '.' to get dim and rest
        size_t dotPos = id.find('.');
        if (dotPos == std::string::npos) {
            // Malformed ID, return defaults
            parsed.dim = 3;
            parsed.trans_digit = 50;
            parsed.harm_digit = 50;
            parsed.fx_digit = 20;
            parsed.tuning_prime = 7;
            parsed.damp_digit = 50;
            parsed.freq_digit = 50;
            parsed.type = 'g';
            return parsed;
        }
        
        parsed.dim = std::stoi(id.substr(0, dotPos));
        std::string rest = id.substr(dotPos + 1);
        
        // Extract type (last character)
        parsed.type = rest.back();
        std::string attrs = rest.substr(0, rest.length() - 1);
        
        // Parse attributes
        if (attrs.length() >= 11) {
            parsed.trans_digit = std::stoi(attrs.substr(0, 2));
            parsed.harm_digit = std::stoi(attrs.substr(2, 2));
            parsed.fx_digit = std::stoi(attrs.substr(4, 2));
            parsed.tuning_prime = std::stoi(attrs.substr(6, 1));
            parsed.damp_digit = std::stoi(attrs.substr(7, 2));
            parsed.freq_digit = std::stoi(attrs.substr(9, 2));
        }
        
        return parsed;
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
            mapping.sectionName = section["sectionName"].get<std::string>();
            mapping.group = section["group"].get<std::string>();
            
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
    json processGuitarInstrument(const json& instrumentData, const std::string& instrumentName) {
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
        
        // Add ID after building result
        std::string id = generateId(result, "guitar");
        result["id"] = id;
        
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
    json processGroupEffect(const json& groupData, const std::string& groupName) {
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
        
        // Add ID after building result
        std::string id = generateId(result, "group");
        result["id"] = id;
        
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
        std::cout << "Calculating AI scores using reference data..." << std::endl;
        
        // Example internal scoring logic
        if (moodsData.contains("moods")) {
            std::cout << "Processing mood reference data for scoring..." << std::endl;
        }
        
        if (synthData.contains("sections")) {
            std::cout << "Processing synthesizer reference data for scoring..." << std::endl;
        }
    }
    
    void calculateLayeringRoles() {
        // This calculates 1-6 layering stages but keeps them internal
        std::cout << "Calculating layering roles (1-6 stages) for internal use..." << std::endl;
        
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
            // Add ID if not already present
            if (!config.contains("id")) {
                finalConfig[name]["id"] = generateId(config, determineCategory(config));
            }
        }
        
        // Process group effects and add them at top level
        json groupEffects = processGroupEffects();
        for (const auto& [name, config] : groupEffects.items()) {
            finalConfig[name] = config;
            // Add ID if not already present
            if (!config.contains("id")) {
                finalConfig[name]["id"] = generateId(config, determineCategory(config));
            }
        }
        
        // Calculate internal AI data (but don't export it)
        calculateAIScores();
        calculateLayeringRoles();
        
        return finalConfig;
    }
    
    // Save configuration to file
    bool saveConfig(const std::string& filename) {
        try {
            json config = generateCleanConfig();
            
            std::ofstream outFile(filename);
            if (!outFile) {
                std::cerr << "Error: Cannot create output file " << filename << std::endl;
                return false;
            }
            
            outFile << config.dump(2);
            outFile.close();
            
            std::cout << "Clean configuration saved to " << filename << std::endl;
            std::cout << "Total instruments/groups processed: " << config.size() << std::endl;
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error saving configuration: " << e.what() << std::endl;
            return false;
        }
    }
    
    // Print summary of what was processed
    void printSummary() {
        std::cout << "\n=== JSON Reader System Summary ===" << std::endl;
        std::cout << "Reference files loaded for AI scoring:" << std::endl;
        std::cout << "  - moods.json: " << (moodsData.empty() ? "Not loaded" : "Loaded") << std::endl;
        std::cout << "  - Synthesizer.json: " << (synthData.empty() ? "Not loaded" : "Loaded") << std::endl;
        
        std::cout << "\nSource files processed for config output:" << std::endl;
        std::cout << "  - guitar.json instruments/articulations processed" << std::endl;
        std::cout << "  - group.json effects processed" << std::endl;
        
        std::cout << "\nSection mappings loaded: " << sectionMappings.size() << std::endl;
        
        std::cout << "\nInternal AI data calculated (not exported):" << std::endl;
        std::cout << "  - AI scores: " << aiScores.size() << " items" << std::endl;
        std::cout << "  - Layering roles: " << layeringRoles.size() << " items" << std::endl;
        std::cout << "=================================" << std::endl;
    }
};

int main() {
    std::cout << "JSON Reader System - Clean Configuration Generator" << std::endl;
    std::cout << "=================================================" << std::endl;
    
    JsonReaderSystem system;
    
    // Load all JSON files
    if (!system.loadJsonFiles(".")) {
        std::cerr << "Failed to load JSON files. Exiting." << std::endl;
        return 1;
    }
    
    // Generate and save clean configuration
    if (!system.saveConfig("clean_config.json")) {
        std::cerr << "Failed to save configuration. Exiting." << std::endl;
        return 1;
    }
    
    // Print summary
    system.printSummary();
    
    // Test ID generation
    json test = system.generateCleanConfig();
    for (auto& [name, config] : test.items()) {
        if (config.contains("id")) {
            std::cout << "\nTest: " << name << " has ID: " << config["id"] << std::endl;
            break;
        }
    }
    
    std::cout << "\nConfiguration generated successfully!" << std::endl;
    std::cout << "Next stage: WAV writer will use this clean config for synthesis." << std::endl;
    
    return 0;
}