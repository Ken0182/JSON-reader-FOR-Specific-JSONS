#include "json_reader_system.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <numeric>

using json = nlohmann::json;

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

class JsonReaderSystem {
private:
    json guitarData, groupData, moodsData, structureData, synthData;
    std::unordered_map<std::string, float> registry;
    
    // ID Generation Core Functions
    std::string generateId(const json& entry, const std::string& entryType) {
        int dim = determineDim(entry, entryType);
        
        // Extract and quantize properties with fallbacks
        float trans_avg = extractTransientIntensity(entry);
        int trans_digit = quantize(trans_avg, 0.0f, 1.0f, 99);
        
        int harm_digit = extractHarmonicComplexity(entry);
        int fx_digit = extractFxComplexity(entry);
        int tuning_prime = extractTuningPrime(entry);
        int damp_digit = extractDynamicRange(entry);
        int freq_digit = extractFrequencyRange(entry);
        
        // Concatenate attributes (6 digits)
        std::string attrs = formatAttributes(trans_digit, harm_digit, fx_digit, 
                                           tuning_prime, damp_digit, freq_digit);
        
        char type = entryType[0]; // i/g/x/m/s
        return std::to_string(dim) + "." + attrs + type;
    }
    
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
                // Log scale: attack < 0.05 = high intensity (80-99)
                if (avg_attack < 0.05f) return 0.85f;
                else if (avg_attack < 0.5f) return 0.6f;
                else return 0.3f;
            }
        }
        
        return 0.5f; // NA default
    }
    
    int extractHarmonicComplexity(const json& entry) {
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
                    if (len < 3) return 25; // low
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
                if (len < 3) return 25;
                else if (len <= 5) return 50;
                else return 75;
            }
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
        char buffer[7];
        snprintf(buffer, sizeof(buffer), "%02d%02d%02d%d%02d%02d", 
                trans, harm, fx, tuning, damp, freq);
        return std::string(buffer);
    }
    
    int quantize(float val, float min, float max, int bins) {
        if (val < min || val > max) return bins / 2;
        return static_cast<int>((val - min) / (max - min) * bins);
    }
    
    // GCD helper for prime compatibility
    int gcd(int a, int b) {
        while (b != 0) {
            int temp = b;
            b = a % b;
            a = temp;
        }
        return a;
    }

public:
    bool loadJsonFiles(const std::string& basePath) {
        try {
            std::ifstream guitarFile(basePath + "/guitar.json");
            std::ifstream groupFile(basePath + "/group.json");
            std::ifstream moodsFile(basePath + "/moods.json");
            std::ifstream structureFile(basePath + "/structure.json");
            std::ifstream synthFile(basePath + "/Synthesizer.json");
            
            if (!guitarFile || !groupFile || !moodsFile || !structureFile || !synthFile) {
                std::cerr << "Error: Could not open one or more JSON files" << std::endl;
                return false;
            }
            
            guitarFile >> guitarData;
            groupFile >> groupData;
            moodsFile >> moodsData;
            structureFile >> structureData;
            synthFile >> synthData;
            
            // Generate IDs for all entries
            generateAllIds();
            
            std::cout << "Successfully loaded and generated IDs for all JSON files" << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Error loading JSON files: " << e.what() << std::endl;
            return false;
        }
    }
    
    void generateAllIds() {
        // Generate IDs for guitar entries
        if (guitarData.contains("groups")) {
            for (auto& [key, group] : guitarData["groups"].items()) {
                group["id"] = generateId(group, "instrument");
                group["type"] = "instrument";
            }
        }
        if (guitarData.contains("articulations")) {
            for (auto& [key, articulation] : guitarData["articulations"].items()) {
                articulation["id"] = generateId(articulation, "instrument");
                articulation["type"] = "instrument";
            }
        }
        
        // Generate IDs for group entries
        if (groupData.contains("groups")) {
            for (auto& [key, group] : groupData["groups"].items()) {
                group["id"] = generateId(group, "group");
                group["type"] = "group";
            }
        }
        
        // Generate IDs for mood entries
        if (moodsData.contains("moods")) {
            for (auto& [key, mood] : moodsData["moods"].items()) {
                mood["id"] = generateId(mood, "mood");
                mood["type"] = "mood";
            }
        }
        
        // Generate IDs for structure entries
        if (structureData.contains("sections")) {
            for (auto& [key, section] : structureData["sections"].items()) {
                section["id"] = generateId(section, "structure");
                section["type"] = "structure";
            }
        }
        
        // Generate IDs for synthesizer entries
        if (synthData.contains("synthesizers")) {
            for (auto& [key, synth] : synthData["synthesizers"].items()) {
                synth["id"] = generateId(synth, "synthesizer");
                synth["type"] = "synthesizer";
            }
        }
    }
    
    json generateCleanConfig() {
        json cleanConfig;
        cleanConfig["metadata"]["version"] = "4Z-1.0";
        cleanConfig["metadata"]["id_system"] = "4Z_dynamic_dimensional";
        cleanConfig["metadata"]["generation_timestamp"] = std::time(nullptr);
        
        // Include all data with IDs
        cleanConfig["guitar"] = guitarData;
        cleanConfig["groups"] = groupData;
        cleanConfig["moods"] = moodsData;
        cleanConfig["structure"] = structureData;
        cleanConfig["synthesizers"] = synthData;
        
        // Add registry for dynamic vectorization
        cleanConfig["registry"] = registry;
        
        return cleanConfig;
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
        
        // Parse 6-digit attributes: TTHHFFXDDFF
        // TT = transients (2 digits)
        // HH = harmonics (2 digits)  
        // FF = fx (2 digits)
        // X = tuning prime (1 digit)
        // DD = damping (2 digits)
        // FF = frequency (2 digits)
        if (attrs.length() >= 9) {
            parsed.trans_digit = std::stoi(attrs.substr(0, 2));
            parsed.harm_digit = std::stoi(attrs.substr(2, 2));
            parsed.fx_digit = std::stoi(attrs.substr(4, 2));
            parsed.tuning_prime = std::stoi(attrs.substr(6, 1));
            parsed.damp_digit = std::stoi(attrs.substr(7, 2));
            parsed.freq_digit = std::stoi(attrs.substr(9, 2));
        }
        
        return parsed;
    }
    
    // Getters for other systems
    const json& getGuitarData() const { return guitarData; }
    const json& getGroupData() const { return groupData; }
    const json& getMoodsData() const { return moodsData; }
    const json& getStructureData() const { return structureData; }
    const json& getSynthData() const { return synthData; }
};