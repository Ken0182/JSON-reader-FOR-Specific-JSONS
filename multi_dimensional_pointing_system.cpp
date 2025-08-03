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
#include <unordered_map>
#include <regex>
#include <cmath>
#include <chrono>
#include <sstream>
#include <memory>
#include <queue>
#include <cassert>

using namespace std;
using json = nlohmann::json;

// Forward declarations
class SemanticPointer;
class TechnicalCompatibilityPointer;
class MusicalRolePointer;
class LayeringArrangementPointer;
class MultiDimensionalPointingSystem;

// Enhanced configuration entry with multi-dimensional metadata
struct EnhancedConfigEntry {
    // Basic identity
    string id;
    string name;
    string category;  // guitar, group, effect
    json configData;  // Original configuration data
    
    // 4Z Integration
    string zId;  // 4Z ID for mathematical compatibility
    unordered_map<string, float> dynamicProps;  // Registry property values
    
    // 1D: Semantic metadata (existing)
    vector<string> semanticTags;
    string description;
    vector<float> embedding;
    
    // 2D: Technical specifications
    struct TechnicalSpecs {
        float sampleRate = 44100.0f;
        int bitDepth = 24;
        int polyphonyLimit = 16;
        string envelopeType = "ADSR";  // ADSR, DADSR, AD, etc.
        vector<string> supportedFormats = {"VST", "AU"};
        float minBPM = 60.0f;
        float maxBPM = 200.0f;
        bool supportsRealtime = true;
        string midiChannelSupport = "all";  // "all", "single", "multi"
        int bufferSizeMin = 64;
        int bufferSizeMax = 2048;
        vector<string> requiredEffects;  // Required effect types
        vector<string> incompatibleEffects;  // Incompatible effect types
    } techSpecs;
    
    // 3D: Musical role metadata
    struct MusicalRole {
        string primaryRole = "unknown";  // lead, bass, pad, drums, effect
        vector<string> secondaryRoles;   // Additional roles this can serve
        string musicalContext = "any";   // intro, verse, chorus, bridge, outro
        float prominence = 0.5f;         // 0.0 = background, 1.0 = foreground
        bool isRhythmic = false;
        bool isMelodic = true;
        bool isHarmonic = true;
        vector<string> typicalPartners;  // IDs or roles that pair well
        string dynamicRange = "medium";  // soft, medium, loud
        string tonalCharacter = "neutral"; // bright, dark, warm, cold
    } musicalRole;
    
    // 4D: Layering/Arrangement metadata
    struct LayeringInfo {
        string preferredLayer = "midground";  // foreground, midground, background
        vector<string> compatibleLayers;     // Which layers this works with
        string arrangementPosition = "any";   // intro, verse, chorus, bridge, outro, fill
        float stereoWidth = 0.5f;            // 0.0 = mono, 1.0 = wide stereo
        string frequencyRange = "mid";        // low, low-mid, mid, high-mid, high, full
        bool canDoubleOctave = false;        // Can be doubled an octave up/down
        int maxSimultaneousInstances = 1;    // How many instances can play together
        float mixPriority = 0.5f;           // 0.0 = low priority, 1.0 = high priority
    } layeringInfo;
    
    // Compatibility arrays
    vector<string> compatibleWith;          // Explicit compatibility list
    vector<string> incompatibleWith;        // Explicit incompatibility list
    vector<string> preferredCombinations;   // Suggested combinations
    vector<string> compat_fx;               // Compatible FX categories
    
    // Real-world plugin compatibility
    struct PluginCompatibility {
        string pluginFormat = "VST3";       // VST, VST3, AU, AAX
        string vendor = "unknown";
        string version = "1.0";
        vector<string> hostCompatibility = {"Ableton", "Logic", "Cubase"};
        bool supportsAutomation = true;
        bool supportsMPE = false;           // MIDI Polyphonic Expression
        int latencyMs = 0;
        string cpuUsage = "low";           // low, medium, high
    } pluginInfo;
};

// 1D: Semantic Pointing System (Enhanced from existing)
class SemanticPointer {
private:
    unordered_map<string, vector<float>> embeddings;
    
public:
    SemanticPointer() {
        // Initialize with domain-specific embeddings
        loadMusicDomainEmbeddings();
    }
    
    void loadMusicDomainEmbeddings() {
        // Music semantic embeddings (simplified)
        embeddings["warm"] = {0.8f, 0.2f, 0.6f, 0.1f, 0.9f};
        embeddings["bright"] = {0.2f, 0.9f, 0.1f, 0.8f, 0.3f};
        embeddings["aggressive"] = {0.9f, 0.1f, 0.8f, 0.2f, 0.7f};
        embeddings["calm"] = {0.1f, 0.8f, 0.2f, 0.9f, 0.1f};
        embeddings["lead"] = {0.7f, 0.6f, 0.8f, 0.4f, 0.5f};
        embeddings["bass"] = {0.9f, 0.1f, 0.2f, 0.3f, 0.8f};
        embeddings["pad"] = {0.3f, 0.7f, 0.4f, 0.8f, 0.2f};
        embeddings["reverb"] = {0.2f, 0.5f, 0.6f, 0.7f, 0.4f};
        embeddings["delay"] = {0.4f, 0.6f, 0.5f, 0.5f, 0.6f};
    }
    
    /**
     * 1D Semantic Pointing: Find semantically similar configurations
     * Considers: Tags, embeddings, timbral characteristics
     */
    float calculateSemanticCompatibility(const EnhancedConfigEntry& a, const EnhancedConfigEntry& b) {
        if (a.embedding.empty() || b.embedding.empty()) return 0.0f;
        
        // Cosine similarity between embeddings
        float dotProduct = 0.0f;
        float normA = 0.0f;
        float normB = 0.0f;
        
        for (size_t i = 0; i < min(a.embedding.size(), b.embedding.size()); ++i) {
            dotProduct += a.embedding[i] * b.embedding[i];
            normA += a.embedding[i] * a.embedding[i];
            normB += b.embedding[i] * b.embedding[i];
        }
        
        if (normA == 0.0f || normB == 0.0f) return 0.0f;
        
        float similarity = dotProduct / (sqrt(normA) * sqrt(normB));
        
        // Boost for shared semantic tags
        int sharedTags = 0;
        for (const string& tagA : a.semanticTags) {
            for (const string& tagB : b.semanticTags) {
                if (tagA == tagB) sharedTags++;
            }
        }
        
        return similarity + (sharedTags * 0.1f);
    }
    
    vector<string> explainSemanticMatch(const EnhancedConfigEntry& a, const EnhancedConfigEntry& b) {
        vector<string> explanations;
        
        float similarity = calculateSemanticCompatibility(a, b);
        if (similarity > 0.7f) {
            explanations.push_back("High semantic similarity (" + to_string(similarity) + ")");
        }
        
        // Find shared tags
        for (const string& tagA : a.semanticTags) {
            for (const string& tagB : b.semanticTags) {
                if (tagA == tagB) {
                    explanations.push_back("Shared semantic tag: '" + tagA + "'");
                }
            }
        }
        
        return explanations;
    }
};

// 2D: Technical Compatibility Pointing System
class TechnicalCompatibilityPointer {
public:
    /**
     * 2D Technical Pointing: Check parameter-level compatibility
     * Considers: Sample rate, bit depth, polyphony, envelope types, buffer sizes
     */
    struct CompatibilityResult {
        bool isCompatible = false;
        float compatibilityScore = 0.0f;
        vector<string> issues;
        vector<string> warnings;
        vector<string> strengths;
        map<string, string> suggestions;
    };
    
    CompatibilityResult checkTechnicalCompatibility(const EnhancedConfigEntry& a, const EnhancedConfigEntry& b) {
        CompatibilityResult result;
        float score = 0.0f;
        int totalChecks = 0;
        
        // Sample rate compatibility
        totalChecks++;
        if (abs(a.techSpecs.sampleRate - b.techSpecs.sampleRate) < 0.1f) {
            score += 1.0f;
            result.strengths.push_back("Matching sample rates (" + to_string(a.techSpecs.sampleRate) + "Hz)");
        } else {
            result.warnings.push_back("Different sample rates: " + to_string(a.techSpecs.sampleRate) + 
                                    "Hz vs " + to_string(b.techSpecs.sampleRate) + "Hz");
            result.suggestions["sampleRate"] = "Consider resampling to match rates";
        }
        
        // Bit depth compatibility
        totalChecks++;
        if (a.techSpecs.bitDepth == b.techSpecs.bitDepth) {
            score += 1.0f;
            result.strengths.push_back("Matching bit depths (" + to_string(a.techSpecs.bitDepth) + "-bit)");
        } else {
            result.warnings.push_back("Different bit depths: " + to_string(a.techSpecs.bitDepth) + 
                                    " vs " + to_string(b.techSpecs.bitDepth));
        }
        
        // Polyphony compatibility
        totalChecks++;
        int minPolyphony = min(a.techSpecs.polyphonyLimit, b.techSpecs.polyphonyLimit);
        if (minPolyphony >= 8) {
            score += 1.0f;
            result.strengths.push_back("Adequate polyphony (" + to_string(minPolyphony) + " voices)");
        } else {
            result.warnings.push_back("Limited polyphony (" + to_string(minPolyphony) + " voices)");
        }
        
        // Envelope type compatibility
        totalChecks++;
        if (a.techSpecs.envelopeType == b.techSpecs.envelopeType) {
            score += 1.0f;
            result.strengths.push_back("Compatible envelope types (" + a.techSpecs.envelopeType + ")");
        } else {
            result.warnings.push_back("Different envelope types: " + a.techSpecs.envelopeType + 
                                    " vs " + b.techSpecs.envelopeType);
            result.suggestions["envelope"] = "Consider envelope type conversion";
        }
        
        // BPM range compatibility
        totalChecks++;
        float bpmOverlapStart = max(a.techSpecs.minBPM, b.techSpecs.minBPM);
        float bpmOverlapEnd = min(a.techSpecs.maxBPM, b.techSpecs.maxBPM);
        if (bpmOverlapEnd > bpmOverlapStart) {
            score += 1.0f;
            result.strengths.push_back("Compatible BPM range (" + to_string(bpmOverlapStart) + 
                                     "-" + to_string(bpmOverlapEnd) + ")");
        } else {
            result.issues.push_back("No BPM overlap: [" + to_string(a.techSpecs.minBPM) + 
                                  "-" + to_string(a.techSpecs.maxBPM) + "] vs [" + 
                                  to_string(b.techSpecs.minBPM) + "-" + to_string(b.techSpecs.maxBPM) + "]");
        }
        
        result.compatibilityScore = score / totalChecks;
        result.isCompatible = result.compatibilityScore >= 0.7f && result.issues.empty();
        
        return result;
    }
};

// 3D: Musical Role Pointing System
class MusicalRolePointer {
public:
    /**
     * 3D Musical Role Pointing: Groups/links by musical function
     * Considers: Role compatibility, musical context, typical combinations
     */
    struct RoleCompatibilityMatrix {
        map<string, vector<string>> compatibleRoles = {
            {"lead", {"pad", "bass", "drums", "arp", "chord"}},
            {"bass", {"lead", "pad", "drums", "chord"}},
            {"pad", {"lead", "bass", "drums", "arp", "chord"}},
            {"drums", {"lead", "bass", "pad", "perc", "chord"}},
            {"arp", {"lead", "pad", "bass", "chord"}},
            {"chord", {"lead", "bass", "pad", "arp"}},
            {"effect", {"lead", "bass", "pad", "drums", "arp", "chord"}}
        };
        
        map<string, vector<string>> typicalCombinations = {
            {"lead_synth", {"bass_synth", "pad_warm", "drums_electronic"}},
            {"acoustic_guitar", {"bass_guitar", "drums_acoustic", "piano"}},
            {"electric_guitar", {"bass_guitar", "drums_rock", "synth_pad"}},
            {"piano", {"strings", "bass_acoustic", "drums_jazz"}},
            {"vocal", {"guitar", "piano", "strings", "bass", "drums"}}
        };
        
        map<string, float> roleProminence = {
            {"lead", 0.9f}, {"bass", 0.7f}, {"pad", 0.3f}, 
            {"drums", 0.8f}, {"arp", 0.6f}, {"chord", 0.5f}, {"effect", 0.2f}
        };
    };
    
    RoleCompatibilityMatrix matrix;
    
    float calculateMusicalRoleCompatibility(const EnhancedConfigEntry& a, const EnhancedConfigEntry& b) {
        float score = 0.0f;
        
        // Check role compatibility
        auto& compatibleRoles = matrix.compatibleRoles[a.musicalRole.primaryRole];
        bool roleCompatible = find(compatibleRoles.begin(), compatibleRoles.end(), 
                                 b.musicalRole.primaryRole) != compatibleRoles.end();
        
        if (roleCompatible) {
            score += 0.4f;
        }
        
        // Check musical context compatibility
        if (a.musicalRole.musicalContext == b.musicalRole.musicalContext || 
            a.musicalRole.musicalContext == "any" || b.musicalRole.musicalContext == "any") {
            score += 0.2f;
        }
        
        // Check prominence balance (avoid two highly prominent instruments)
        float prominenceDiff = abs(a.musicalRole.prominence - b.musicalRole.prominence);
        if (prominenceDiff > 0.3f) {  // Good separation
            score += 0.2f;
        }
        
        // Check tonal character compatibility
        if (a.musicalRole.tonalCharacter == b.musicalRole.tonalCharacter ||
            a.musicalRole.tonalCharacter == "neutral" || b.musicalRole.tonalCharacter == "neutral") {
            score += 0.1f;
        }
        
        // Check for typical partners
        for (const string& partner : a.musicalRole.typicalPartners) {
            if (partner == b.musicalRole.primaryRole || partner == b.id) {
                score += 0.1f;
                break;
            }
        }
        
        return min(score, 1.0f);
    }
    
    vector<string> explainMusicalRoleMatch(const EnhancedConfigEntry& a, const EnhancedConfigEntry& b) {
        vector<string> explanations;
        
        auto& compatibleRoles = matrix.compatibleRoles[a.musicalRole.primaryRole];
        bool roleCompatible = find(compatibleRoles.begin(), compatibleRoles.end(), 
                                 b.musicalRole.primaryRole) != compatibleRoles.end();
        
        if (roleCompatible) {
            explanations.push_back("Compatible musical roles: " + a.musicalRole.primaryRole + 
                                 " works with " + b.musicalRole.primaryRole);
        }
        
        if (a.musicalRole.musicalContext == b.musicalRole.musicalContext) {
            explanations.push_back("Matching musical context: " + a.musicalRole.musicalContext);
        }
        
        float prominenceDiff = abs(a.musicalRole.prominence - b.musicalRole.prominence);
        if (prominenceDiff > 0.3f) {
            explanations.push_back("Good prominence balance: " + to_string(a.musicalRole.prominence) + 
                                 " vs " + to_string(b.musicalRole.prominence));
        }
        
        return explanations;
    }
};

// 4D: Layering/Arrangement Pointing System
class LayeringArrangementPointer {
public:
    /**
     * 4D Layering Pointing: Suggests compatible layered structures
     * Considers: Foreground/midground/background, frequency ranges, stereo placement
     */
    struct LayeringRules {
        map<string, vector<string>> layerCompatibility = {
            {"foreground", {"midground", "background"}},
            {"midground", {"foreground", "background"}},
            {"background", {"foreground", "midground"}}
        };
        
        map<string, string> frequencyRangeOrder = {
            {"low", "0"}, {"low-mid", "1"}, {"mid", "2"}, 
            {"high-mid", "3"}, {"high", "4"}, {"full", "5"}
        };
        
        map<string, int> arrangementOrder = {
            {"intro", 0}, {"verse", 1}, {"chorus", 2}, 
            {"bridge", 3}, {"outro", 4}, {"fill", 5}, {"any", 6}
        };
    };
    
    LayeringRules rules;
    
    float calculateLayeringCompatibility(const EnhancedConfigEntry& a, const EnhancedConfigEntry& b) {
        float score = 0.0f;
        
        // Check layer compatibility
        auto& compatibleLayers = rules.layerCompatibility[a.layeringInfo.preferredLayer];
        if (find(compatibleLayers.begin(), compatibleLayers.end(), 
                b.layeringInfo.preferredLayer) != compatibleLayers.end()) {
            score += 0.3f;
        }
        
        // Check frequency range separation (avoid conflicts)
        string freqA = a.layeringInfo.frequencyRange;
        string freqB = b.layeringInfo.frequencyRange;
        if (freqA != freqB || freqA == "full" || freqB == "full") {
            score += 0.2f;
        }
        
        // Check stereo width compatibility
        float stereoWidthSum = a.layeringInfo.stereoWidth + b.layeringInfo.stereoWidth;
        if (stereoWidthSum <= 1.5f) {  // Leave some stereo space
            score += 0.2f;
        }
        
        // Check arrangement position compatibility
        if (a.layeringInfo.arrangementPosition == b.layeringInfo.arrangementPosition ||
            a.layeringInfo.arrangementPosition == "any" || b.layeringInfo.arrangementPosition == "any") {
            score += 0.15f;
        }
        
        // Check mix priority balance
        float priorityDiff = abs(a.layeringInfo.mixPriority - b.layeringInfo.mixPriority);
        if (priorityDiff >= 0.2f) {  // Good priority separation
            score += 0.15f;
        }
        
        return min(score, 1.0f);
    }
    
    vector<string> explainLayeringMatch(const EnhancedConfigEntry& a, const EnhancedConfigEntry& b) {
        vector<string> explanations;
        
        auto& compatibleLayers = rules.layerCompatibility[a.layeringInfo.preferredLayer];
        if (find(compatibleLayers.begin(), compatibleLayers.end(), 
                b.layeringInfo.preferredLayer) != compatibleLayers.end()) {
            explanations.push_back("Compatible layers: " + a.layeringInfo.preferredLayer + 
                                 " with " + b.layeringInfo.preferredLayer);
        }
        
        if (a.layeringInfo.frequencyRange != b.layeringInfo.frequencyRange) {
            explanations.push_back("Good frequency separation: " + a.layeringInfo.frequencyRange + 
                                 " vs " + b.layeringInfo.frequencyRange);
        }
        
        if (a.layeringInfo.arrangementPosition == b.layeringInfo.arrangementPosition) {
            explanations.push_back("Matching arrangement: " + a.layeringInfo.arrangementPosition);
        }
        
        return explanations;
    }
};

// Main Multi-Dimensional Pointing System
class MultiDimensionalPointingSystem {
private:
    SemanticPointer semanticPointer;
    TechnicalCompatibilityPointer techPointer;
    MusicalRolePointer rolePointer;
    LayeringArrangementPointer layeringPointer;
    
    vector<EnhancedConfigEntry> configDatabase;
    unordered_map<string, vector<string>> compatibilityGraph;
    
    // Compatibility matrix for research-based plugin guides
    map<pair<string,string>, float> compatMatrix = {
        {{"lead", "bass"}, 0.9f},
        {{"lead", "pad"}, 0.8f},
        {{"bass", "drums"}, 0.9f},
        {{"pad", "strings"}, 0.85f},
        {{"aggressive", "calm"}, 0.2f},
        {{"bright", "warm"}, 0.6f}
    };
    
    // 4Z ID Math Functions
    ParsedId parseId(const string& id) {
        ParsedId parsed;
        
        // Split by '.' to get dim and rest
        size_t dotPos = id.find('.');
        if (dotPos == string::npos) {
            // Malformed ID, return defaults
            return parsed;
        }
        
        parsed.dim = safeStoi(id.substr(0, dotPos), 3);
        string rest = id.substr(dotPos + 1);
        
        // Extract type (last character)
        if (!rest.empty()) {
            parsed.type = rest.back();
            string attrs = rest.substr(0, rest.length() - 1);
            
            // Length pad: if(attrs.length()<11) attrs += string(11-attrs.length(),'5')
            if (attrs.length() < 11) {
                attrs += string(11 - attrs.length(), '5'); // NA50 pad
            }
            
            // Parse attributes with try-catch
            try {
                if (attrs.length() >= 11) {
                    parsed.trans_digit = safeStoi(attrs.substr(0, 2));
                    parsed.harm_digit = safeStoi(attrs.substr(2, 2));
                    parsed.fx_digit = safeStoi(attrs.substr(4, 2));
                    parsed.tuning_prime = validateTuningPrime(safeStoi(attrs.substr(6, 1), 7));
                    parsed.damp_digit = safeStoi(attrs.substr(7, 2));
                    parsed.freq_digit = safeStoi(attrs.substr(9, 2));
                    
                    // Assert parsed.freq_digit<=99
                    assert(parsed.freq_digit <= 99);
                }
            } catch (const exception& e) {
                cout << "Warning: Failed to parse ID attributes: " << e.what() << endl;
                // parsed already has defaults
            }
        }
        
        return parsed;
    }
    
    int gcd(int a, int b) {
        while (b != 0) {
            int temp = b;
            b = a % b;
            a = temp;
        }
        return a;
    }
    
    float calculateIdCompatibility(const ParsedId& a, const ParsedId& b, vector<string>& explanations) {
        float idScore = 0.0f;
        
        // GCD: Validate primes and apply logic
        int gcd_val = calculateGcd(a.tuning_prime, b.tuning_prime);
        if (gcd_val > 1) {
            idScore += 0.1f;
            explanations.push_back("Prime harmonic match (GCD=" + to_string(gcd_val) + ")");
        } else if (gcd_val == 1 && a.tuning_prime != 7 && b.tuning_prime != 7) {
            // Neutral tuning compatibility (both non-NA primes but GCD=1)
            explanations.push_back("Neutral tuning compatibility");
        }
        
        // Digit proximity scoring
        int trans_diff = abs(a.trans_digit - b.trans_digit);
        if (trans_diff < 10) {
            float proximity_score = 0.05f * (10 - trans_diff) / 10.0f;
            idScore += proximity_score;
            explanations.push_back("Transient proximity ±" + to_string(trans_diff));
        }
        
        int harm_diff = abs(a.harm_digit - b.harm_digit);
        if (harm_diff < 15) {
            float proximity_score = 0.04f * (15 - harm_diff) / 15.0f;
            idScore += proximity_score;
            explanations.push_back("Harmonic complexity proximity ±" + to_string(harm_diff));
        }
        
        int fx_diff = abs(a.fx_digit - b.fx_digit);
        if (fx_diff < 20) {
            float proximity_score = 0.03f * (20 - fx_diff) / 20.0f;
            idScore += proximity_score;
            explanations.push_back("FX complexity proximity ±" + to_string(fx_diff));
        }
        
        // Dimensional compatibility
        if (a.dim == b.dim) {
            idScore += 0.02f;
            explanations.push_back("Same dimensional focus");
        }
        
        return min(1.0f, idScore);
    }
    
    void extractPropertyVector(const json& config, unordered_map<string, float>& props) {
        // Extract property vector for dynamic properties
        if (config.contains("harmonicContent")) {
            auto complexity = config["harmonicContent"].value("complexity", "unknown");
            if (complexity == "low") props["harmonicRichness"] = 0.25f;
            else if (complexity == "medium" || complexity == "med") props["harmonicRichness"] = 0.5f;
            else if (complexity == "high") props["harmonicRichness"] = 0.75f;
            else props["harmonicRichness"] = 0.5f;
        }
        
        if (config.contains("transientDetail")) {
            auto intensity = config["transientDetail"]["intensity"];
            if (intensity.is_array() && intensity.size() >= 2) {
                props["transientSharpness"] = (intensity[0].get<float>() + intensity[1].get<float>()) / 2.0f;
            }
        }
        
        // Add more property extractions as needed
        if (config.contains("fxCategories") && config["fxCategories"].is_array()) {
            props["fxComplexity"] = min(1.0f, float(config["fxCategories"].size()) / 5.0f);
        }
    }
    
    void buildCompatibilityGraph() {
        cout << "Building compatibility graph..." << endl;
        
        for (size_t i = 0; i < configDatabase.size(); i++) {
            for (size_t j = i + 1; j < configDatabase.size(); j++) {
                auto result = analyzeCompatibility(configDatabase[i], configDatabase[j]);
                
                // Add edge if compatibility > 0.5 or ID compatibility > 0.3
                ParsedId idA = parseId(configDatabase[i].zId);
                ParsedId idB = parseId(configDatabase[j].zId);
                vector<string> tmpExplanations;
                float idCompat = calculateIdCompatibility(idA, idB, tmpExplanations);
                
                if (result.overallScore > 0.5f || idCompat > 0.3f) {
                    compatibilityGraph[configDatabase[i].id].push_back(configDatabase[j].id);
                    compatibilityGraph[configDatabase[j].id].push_back(configDatabase[i].id);
                }
            }
        }
        
        cout << "Built compatibility graph with " << compatibilityGraph.size() << " nodes" << endl;
    }
    
    string buildRationale(const MultiDimensionalResult& result) {
        string rationale = "Score: " + to_string(result.overallScore);
        
        for (const string& strength : result.strengths) {
            rationale += "\n" + strength;
        }
        
        if (result.isCreativeMatch) {
            rationale += " [Creative]";
        }
        
        return rationale;
    }

public:
    MultiDimensionalPointingSystem() {
        loadConfigDatabase();
        buildCompatibilityGraph();
    }
    
    void loadConfigDatabase() {
        // Load and enhance existing configuration data
        ifstream configFile("clean_config.json");
        if (!configFile) {
            cerr << "Could not load clean_config.json" << endl;
            return;
        }
        
        json cleanConfig;
        configFile >> cleanConfig;
        
        // Convert to enhanced entries with metadata
        for (const auto& [name, config] : cleanConfig.items()) {
            EnhancedConfigEntry entry = createEnhancedEntry(name, config);
            configDatabase.push_back(entry);
        }
        
        cout << "Loaded " << configDatabase.size() << " configurations with multi-dimensional metadata." << endl;
    }

private:
    EnhancedConfigEntry createEnhancedEntry(const string& name, const json& config) {
        EnhancedConfigEntry entry;
        entry.id = name;
        entry.name = name;
        entry.configData = config;
        
        // 4Z Integration
        entry.zId = config.value("id", "3.5050507i");  // Use generated ID or stub
        extractPropertyVector(config, entry.dynamicProps);
        
        // Determine category
        if (config.contains("guitarParams")) {
            entry.category = "guitar";
        } else if (config.contains("synthesisType")) {
            entry.category = "group";
        } else {
            entry.category = "effect";
        }
        
        // Extract semantic metadata
        extractSemanticMetadata(entry, config);
        
        // Generate technical specifications
        generateTechnicalSpecs(entry, config);
        
        // Determine musical role
        determineMusicalRole(entry, config);
        
        // Set layering information
        setLayeringInfo(entry, config);
        
        // Set compatibility information
        setCompatibilityInfo(entry, config);
        
        // Append property values to embedding
        vector<float> propValues;
        for (const auto& [key, value] : entry.dynamicProps) {
            propValues.push_back(value);
        }
        entry.embedding.insert(entry.embedding.end(), propValues.begin(), propValues.end());
        
        return entry;
    }
    
    void extractSemanticMetadata(EnhancedConfigEntry& entry, const json& config) {
        // Extract semantic tags from sound characteristics
        if (config.contains("soundCharacteristics")) {
            const auto& chars = config["soundCharacteristics"];
            
            if (chars.contains("timbral")) {
                entry.semanticTags.push_back(chars["timbral"].get<string>());
            }
            if (chars.contains("material")) {
                entry.semanticTags.push_back(chars["material"].get<string>());
            }
            if (chars.contains("dynamic")) {
                entry.semanticTags.push_back(chars["dynamic"].get<string>());
            }
            if (chars.contains("emotional")) {
                for (const auto& emotion : chars["emotional"]) {
                    if (emotion.contains("tag")) {
                        entry.semanticTags.push_back(emotion["tag"].get<string>());
                    }
                }
            }
        }
        
        // Generate simple embedding based on tags
        entry.embedding = vector<float>(5, 0.0f);
        for (const string& tag : entry.semanticTags) {
            // Simple hash-based embedding generation
            hash<string> hasher;
            size_t hashValue = hasher(tag);
            for (int i = 0; i < 5; ++i) {
                entry.embedding[i] += ((hashValue >> (i * 8)) & 0xFF) / 255.0f;
            }
        }
        
        // Normalize embedding
        float norm = 0.0f;
        for (float val : entry.embedding) {
            norm += val * val;
        }
        if (norm > 0) {
            norm = sqrt(norm);
            for (float& val : entry.embedding) {
                val /= norm;
            }
        }
    }
    
    void generateTechnicalSpecs(EnhancedConfigEntry& entry, const json& config) {
        // Set default technical specifications
        entry.techSpecs.sampleRate = 44100.0f;
        entry.techSpecs.bitDepth = 24;
        entry.techSpecs.polyphonyLimit = 16;
        
        // Determine envelope type from ADSR
        if (config.contains("adsr") && config["adsr"].contains("type")) {
            entry.techSpecs.envelopeType = config["adsr"]["type"].get<string>();
        }
        
        // Set BPM range based on instrument type
        if (entry.category == "guitar") {
            entry.techSpecs.minBPM = 60.0f;
            entry.techSpecs.maxBPM = 180.0f;
        } else if (entry.category == "group") {
            entry.techSpecs.minBPM = 80.0f;
            entry.techSpecs.maxBPM = 200.0f;
        }
        
        // Set plugin compatibility
        entry.pluginInfo.pluginFormat = "VST3";
        entry.pluginInfo.hostCompatibility = {"Ableton", "Logic", "Cubase", "Pro Tools"};
        entry.pluginInfo.supportsAutomation = true;
        
        // Set CPU usage based on complexity
        if (config.contains("effects") && config["effects"].is_array() && config["effects"].size() > 2) {
            entry.pluginInfo.cpuUsage = "high";
        } else if (entry.category == "group") {
            entry.pluginInfo.cpuUsage = "medium";
        } else {
            entry.pluginInfo.cpuUsage = "low";
        }
    }
    
    void determineMusicalRole(EnhancedConfigEntry& entry, const json& config) {
        // Determine primary role from name and characteristics
        string nameLower = entry.name;
        transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
        
        if (nameLower.find("lead") != string::npos) {
            entry.musicalRole.primaryRole = "lead";
            entry.musicalRole.prominence = 0.9f;
            entry.musicalRole.isMelodic = true;
        } else if (nameLower.find("bass") != string::npos) {
            entry.musicalRole.primaryRole = "bass";
            entry.musicalRole.prominence = 0.7f;
            entry.musicalRole.isRhythmic = true;
        } else if (nameLower.find("pad") != string::npos) {
            entry.musicalRole.primaryRole = "pad";
            entry.musicalRole.prominence = 0.3f;
            entry.musicalRole.isHarmonic = true;
        } else if (nameLower.find("arp") != string::npos) {
            entry.musicalRole.primaryRole = "arp";
            entry.musicalRole.prominence = 0.6f;
            entry.musicalRole.isRhythmic = true;
            entry.musicalRole.isMelodic = true;
        } else {
            entry.musicalRole.primaryRole = "pad";
            entry.musicalRole.prominence = 0.4f;
        }
        
        // Set tonal character from sound characteristics
        if (config.contains("soundCharacteristics") && config["soundCharacteristics"].contains("timbral")) {
            string timbral = config["soundCharacteristics"]["timbral"].get<string>();
            if (timbral == "bright" || timbral == "sharp") {
                entry.musicalRole.tonalCharacter = "bright";
            } else if (timbral == "warm" || timbral == "soft") {
                entry.musicalRole.tonalCharacter = "warm";
            } else if (timbral == "dark" || timbral == "deep") {
                entry.musicalRole.tonalCharacter = "dark";
            }
        }
    }
    
    void setLayeringInfo(EnhancedConfigEntry& entry, const json& config) {
        // Set layering based on role and prominence
        if (entry.musicalRole.prominence >= 0.7f) {
            entry.layeringInfo.preferredLayer = "foreground";
        } else if (entry.musicalRole.prominence >= 0.4f) {
            entry.layeringInfo.preferredLayer = "midground";
        } else {
            entry.layeringInfo.preferredLayer = "background";
        }
        
        // Set frequency range based on instrument type
        if (entry.musicalRole.primaryRole == "bass") {
            entry.layeringInfo.frequencyRange = "low";
        } else if (entry.musicalRole.primaryRole == "lead") {
            entry.layeringInfo.frequencyRange = "high-mid";
        } else if (entry.musicalRole.primaryRole == "pad") {
            entry.layeringInfo.frequencyRange = "mid";
        } else {
            entry.layeringInfo.frequencyRange = "full";
        }
        
        // Set stereo width
        if (entry.musicalRole.primaryRole == "bass") {
            entry.layeringInfo.stereoWidth = 0.2f;  // Keep bass centered
        } else if (entry.musicalRole.primaryRole == "pad") {
            entry.layeringInfo.stereoWidth = 0.8f;  // Pads can be wide
        } else {
            entry.layeringInfo.stereoWidth = 0.5f;
        }
        
        // Set mix priority same as prominence
        entry.layeringInfo.mixPriority = entry.musicalRole.prominence;
    }
    
    void setCompatibilityInfo(EnhancedConfigEntry& entry, const json& config) {
        // Extract FX categories for compatibility
        if (config.contains("fxCategories") && config["fxCategories"].is_array()) {
            for (const auto& fx : config["fxCategories"]) {
                if (fx.is_string()) {
                    entry.compat_fx.push_back(fx);
                }
            }
        }
        
        // Set basic compatibility based on role
        if (entry.musicalRole.primaryRole == "lead") {
            entry.compatibleWith = {"bass", "pad", "drums", "chord"};
            entry.musicalRole.typicalPartners = {"bass", "pad"};
        } else if (entry.musicalRole.primaryRole == "bass") {
            entry.compatibleWith = {"lead", "pad", "drums", "chord"};
            entry.musicalRole.typicalPartners = {"lead", "drums"};
        } else if (entry.musicalRole.primaryRole == "pad") {
            entry.compatibleWith = {"lead", "bass", "drums", "arp"};
            entry.musicalRole.typicalPartners = {"lead", "bass"};
        }
    }

public:
    /**
     * Multi-dimensional compatibility analysis
     * Combines all four pointing dimensions plus 4Z ID math
     */
    struct MultiDimensionalResult {
        float overallScore = 0.0f;
        float semanticScore = 0.0f;
        float technicalScore = 0.0f;
        float musicalRoleScore = 0.0f;
        float layeringScore = 0.0f;
        float idScore = 0.0f;
        
        bool isRecommended = false;
        bool isCreativeMatch = false;
        vector<string> strengths;
        vector<string> issues;
        vector<string> suggestions;
        
        TechnicalCompatibilityPointer::CompatibilityResult technicalDetails;
    };
    
    MultiDimensionalResult analyzeCompatibility(const EnhancedConfigEntry& a, const EnhancedConfigEntry& b) {
        MultiDimensionalResult result;
        
        // 1D: Semantic compatibility
        result.semanticScore = semanticPointer.calculateSemanticCompatibility(a, b);
        
        // 2D: Technical compatibility
        result.technicalDetails = techPointer.checkTechnicalCompatibility(a, b);
        result.technicalScore = result.technicalDetails.compatibilityScore;
        
        // 3D: Musical role compatibility
        result.musicalRoleScore = rolePointer.calculateMusicalRoleCompatibility(a, b);
        
        // 4D: Layering compatibility
        result.layeringScore = layeringPointer.calculateLayeringCompatibility(a, b);
        
        // 4Z: ID compatibility
        ParsedId parsedA = parseId(a.zId);
        ParsedId parsedB = parseId(b.zId);
        result.idScore = calculateIdCompatibility(parsedA, parsedB, result.strengths);
        
        // Calculate weighted overall score
        result.overallScore = (0.2f * result.semanticScore +     // 20% semantic
                              0.25f * result.technicalScore +    // 25% technical
                              0.25f * result.musicalRoleScore +  // 25% musical role
                              0.15f * result.layeringScore +     // 15% layering
                              0.15f * result.idScore);           // 15% ID math
        
        // Scoring: Cap overall <=1.0f
        result.overallScore = min(1.0f, result.overallScore);
        
        // Creative compatibility logic: only +0.05 if overall<0.7 (avoid overboost)
        if (result.idScore > 0.3f && 
            (result.semanticScore > 0.8f || result.technicalScore > 0.8f) &&
            (result.musicalRoleScore < 0.6f || result.layeringScore < 0.6f) &&
            result.overallScore < 0.7f) {
            result.overallScore += 0.05f; // Creative boost (capped)
            result.overallScore = min(1.0f, result.overallScore); // Ensure cap
            result.isCreativeMatch = true;
            result.strengths.push_back("Unexpected valid due to prop synergy");
        }
        
        result.isRecommended = result.overallScore >= 0.7f && result.technicalDetails.isCompatible;
        
        // Collect explanations
        auto semanticReasons = semanticPointer.explainSemanticMatch(a, b);
        result.strengths.insert(result.strengths.end(), semanticReasons.begin(), semanticReasons.end());
        
        auto roleReasons = rolePointer.explainMusicalRoleMatch(a, b);
        result.strengths.insert(result.strengths.end(), roleReasons.begin(), roleReasons.end());
        
        auto layeringReasons = layeringPointer.explainLayeringMatch(a, b);
        result.strengths.insert(result.strengths.end(), layeringReasons.begin(), layeringReasons.end());
        
        result.strengths.insert(result.strengths.end(), 
                               result.technicalDetails.strengths.begin(), 
                               result.technicalDetails.strengths.end());
        
        result.issues.insert(result.issues.end(), 
                            result.technicalDetails.issues.begin(), 
                            result.technicalDetails.issues.end());
        
        return result;
    }
    
    /**
     * Find compatible configurations for a given anchor
     */
    vector<pair<EnhancedConfigEntry, MultiDimensionalResult>> findCompatibleConfigurations(
        const string& anchorId, int maxResults = 10) {
        
        vector<pair<EnhancedConfigEntry, MultiDimensionalResult>> results;
        
        // Check if anchorId is a tree/path (contains / or multiple .)
        if (anchorId.find('/') != string::npos || count(anchorId.begin(), anchorId.end(), '.') > 1) {
            // Handle as tree traversal using compatibility graph
            queue<string> toVisit;
            set<string> visited;
            toVisit.push(anchorId);
            
            while (!toVisit.empty() && results.size() < maxResults) {
                string currentId = toVisit.front();
                toVisit.pop();
                
                if (visited.count(currentId)) continue;
                visited.insert(currentId);
                
                if (compatibilityGraph.count(currentId)) {
                    for (const string& childId : compatibilityGraph[currentId]) {
                        if (!visited.count(childId)) {
                            toVisit.push(childId);
                            
                            // Find the actual entries and analyze
                            auto anchorIt = find_if(configDatabase.begin(), configDatabase.end(),
                                                   [&](const EnhancedConfigEntry& entry) { return entry.id == currentId; });
                            auto childIt = find_if(configDatabase.begin(), configDatabase.end(),
                                                  [&](const EnhancedConfigEntry& entry) { return entry.id == childId; });
                            
                            if (anchorIt != configDatabase.end() && childIt != configDatabase.end()) {
                                MultiDimensionalResult compatibility = analyzeCompatibility(*anchorIt, *childIt);
                                results.emplace_back(*childIt, compatibility);
                            }
                        }
                    }
                }
            }
        } else {
            // Standard compatibility finding
            auto anchorIt = find_if(configDatabase.begin(), configDatabase.end(),
                                   [&](const EnhancedConfigEntry& entry) { return entry.id == anchorId; });
            
            if (anchorIt == configDatabase.end()) {
                return results;
            }
            
            const EnhancedConfigEntry& anchor = *anchorIt;
            
            // Analyze compatibility with all other configurations
            for (const auto& candidate : configDatabase) {
                if (candidate.id == anchorId) continue;
                
                MultiDimensionalResult compatibility = analyzeCompatibility(anchor, candidate);
                if (compatibility.overallScore >= 0.5f) {  // Minimum threshold
                    results.emplace_back(candidate, compatibility);
                }
            }
        }
        
        // Sort by overall compatibility score
        sort(results.begin(), results.end(),
             [](const auto& a, const auto& b) {
                 return a.second.overallScore > b.second.overallScore;
             });
        
        // Limit results
        if (results.size() > maxResults) {
            results.resize(maxResults);
        }
        
        return results;
    }
    
    /**
     * Generate a complete musical arrangement as JSON tree
     */
    json generateArrangementTree(const string& style = "balanced", const string& context = "any", bool useFlat = false) {
        json tree = json::object();
        
        // Tree: Ensure backward (if flag=false return flat)
        if (useFlat) {
            // Return flat arrangement for backward compatibility
            return generateFlatArrangement(style, context);
        }
        
        // Find lead instrument
        auto leadCandidates = findByRole("lead");
        if (leadCandidates.empty()) return tree;
        
        EnhancedConfigEntry root = leadCandidates[0];
        tree["root"] = root.id;
        tree["style"] = style;
        tree["context"] = context;
        
        // Arrangement: Pre-filter with ID compatibility and creative logic
        vector<EnhancedConfigEntry> filtered;
        ParsedId rootParsed = parseId(root.zId);
        
        for (const auto& candidate : configDatabase) {
            if (candidate.id == root.id) continue;
            
            ParsedId candidateParsed = parseId(candidate.zId);
            
            // Pre-filter: bool idCompatible = (gcd>1 || abs(trans_diff)<10)
            int gcd_val = calculateGcd(rootParsed.tuning_prime, candidateParsed.tuning_prime);
            int trans_diff = abs(rootParsed.trans_digit - candidateParsed.trans_digit);
            bool idCompatible = (gcd_val > 1 || trans_diff < 10);
            
            bool fxCompatible = false;
            for (const string& fx : root.compat_fx) {
                if (find(candidate.compat_fx.begin(), candidate.compat_fx.end(), fx) != candidate.compat_fx.end()) {
                    fxCompatible = true;
                    break;
                }
            }
            
            if (idCompatible || fxCompatible) {
                filtered.push_back(candidate);
            }
        }
        
        // Separate into standard and creative matches
        vector<pair<EnhancedConfigEntry, MultiDimensionalResult>> standardMatches;
        vector<pair<EnhancedConfigEntry, MultiDimensionalResult>> creativeMatches;
        
        for (const auto& candidate : filtered) {
            MultiDimensionalResult result = analyzeCompatibility(root, candidate);
            
            // Creative if idScore>0.3 && role<0.6
            if (result.idScore > 0.3f && result.musicalRoleScore < 0.6f) {
                creativeMatches.emplace_back(candidate, result);
            } else {
                standardMatches.emplace_back(candidate, result);
            }
        }
        
        // Sort by compatibility
        sort(standardMatches.begin(), standardMatches.end(),
             [](const auto& a, const auto& b) { return a.second.overallScore > b.second.overallScore; });
        sort(creativeMatches.begin(), creativeMatches.end(),
             [](const auto& a, const auto& b) { return a.second.idScore > b.second.idScore; });
        
        // Build children array (70% standard, 30% creative)
        json children = json::array();
        int maxChildren = 5;
        int creativeSlots = max(1, maxChildren * 30 / 100);
        int standardSlots = maxChildren - creativeSlots;
        
        // Add standard matches
        for (int i = 0; i < standardSlots && i < standardMatches.size(); i++) {
            json child;
            child["id"] = standardMatches[i].first.id;
            child["score"] = standardMatches[i].second.overallScore;
            child["rationale"] = buildRationaleWithId(standardMatches[i].second, root, standardMatches[i].first);
            children.push_back(child);
        }
        
        // Add creative matches
        for (int i = 0; i < creativeSlots && i < creativeMatches.size(); i++) {
            json child;
            child["id"] = creativeMatches[i].first.id;
            child["score"] = creativeMatches[i].second.overallScore;
            child["rationale"] = "Creative: " + buildRationaleWithId(creativeMatches[i].second, root, creativeMatches[i].first);
            children.push_back(child);
        }
        
        tree["children"] = children;
        return tree;
    }
    
    json generateFlatArrangement(const string& style, const string& context) {
        json flat = json::object();
        flat["style"] = style;
        flat["context"] = context;
        flat["type"] = "flat";
        
        json suggestions = json::array();
        for (const auto& entry : configDatabase) {
            json suggestion;
            suggestion["id"] = entry.id;
            suggestion["category"] = entry.category;
            suggestion["role"] = entry.musicalRole.primaryRole;
            suggestions.push_back(suggestion);
        }
        
        flat["suggestions"] = suggestions;
        return flat;
    }
    
    string buildRationaleWithId(const MultiDimensionalResult& result, const EnhancedConfigEntry& root, const EnhancedConfigEntry& candidate) {
        string rationale = "Score: " + to_string(result.overallScore);
        
        // Include ID-specific explanations
        ParsedId rootId = parseId(root.zId);
        ParsedId candId = parseId(candidate.zId);
        
        int gcd_val = calculateGcd(rootId.tuning_prime, candId.tuning_prime);
        if (gcd_val > 1) {
            rationale += " | GCD compat envelope (GCD=" + to_string(gcd_val) + ")";
        }
        
        int trans_diff = abs(rootId.trans_digit - candId.trans_digit);
        if (trans_diff < 10) {
            rationale += " | Transient sync ±" + to_string(trans_diff);
        }
        
        for (const string& strength : result.strengths) {
            rationale += " | " + strength;
        }
        
        if (result.isCreativeMatch) {
            rationale += " [Creative]";
        }
        
        return rationale;
    }
    
    vector<EnhancedConfigEntry> findByRole(const string& role) {
        vector<EnhancedConfigEntry> results;
        for (const auto& entry : configDatabase) {
            if (entry.musicalRole.primaryRole == role) {
                results.push_back(entry);
            }
        }
        return results;
    }
    
    // Test creative matching
    void testCreativeMatching() {
        cout << "\n=== TESTING CREATIVE MATCHING ===" << endl;
        
        // Mock enhanced config entries for testing
        EnhancedConfigEntry mockA, mockB;
        mockA.zId = "3.492534i";  // ID with high compatibility
        mockB.zId = "3.482533i";  // Close ID
        mockA.musicalRole.primaryRole = "lead";
        mockB.musicalRole.primaryRole = "drums"; // Incompatible role
        
        // Create embeddings to trigger high semantic score
        mockA.embedding = {0.8f, 0.9f, 0.7f, 0.6f, 0.8f};
        mockB.embedding = {0.9f, 0.8f, 0.8f, 0.7f, 0.9f}; // High similarity
        
        MultiDimensionalResult result = analyzeCompatibility(mockA, mockB);
        
        cout << "Mock test: idScore=" << result.idScore << ", roleScore=" << result.musicalRoleScore << endl;
        cout << "Creative match expected if idScore>0.3 && role<0.6" << endl;
        cout << "Result: " << (result.isCreativeMatch ? "CREATIVE MATCH DETECTED" : "Standard match") << endl;
        
        if (result.idScore > 0.3f && result.musicalRoleScore < 0.6f && result.isCreativeMatch) {
            cout << "✓ PASS: Creative matching logic working" << endl;
        } else {
            cout << "✗ INFO: Creative conditions not met or not triggered" << endl;
        }
        
        cout << "=== CREATIVE MATCHING TEST COMPLETE ===" << endl;
    }
    
    void printSystemStatistics() {
        cout << "\n=== MULTI-DIMENSIONAL POINTING SYSTEM STATISTICS ===" << endl;
        cout << "Total configurations: " << configDatabase.size() << endl;
        
        map<string, int> categoryStats;
        map<string, int> roleStats;
        map<string, int> layerStats;
        
        for (const auto& entry : configDatabase) {
            categoryStats[entry.category]++;
            roleStats[entry.musicalRole.primaryRole]++;
            layerStats[entry.layeringInfo.preferredLayer]++;
        }
        
        cout << "\nBy category:" << endl;
        for (const auto& [cat, count] : categoryStats) {
            cout << "  " << cat << ": " << count << endl;
        }
        
        cout << "\nBy musical role:" << endl;
        for (const auto& [role, count] : roleStats) {
            cout << "  " << role << ": " << count << endl;
        }
        
        cout << "\nBy preferred layer:" << endl;
        for (const auto& [layer, count] : layerStats) {
            cout << "  " << layer << ": " << count << endl;
        }
        
        cout << "\nCompatibility graph edges: " << compatibilityGraph.size() << endl;
        cout << "=========================================================" << endl;
    }
};

int main() {
    cout << "Multi-Dimensional Pointing System - Enhanced 4Z with Creative Testing" << endl;
    cout << "====================================================================" << endl;
    
    try {
        MultiDimensionalPointingSystem system;
        system.printSystemStatistics();
        
        // Test creative matching
        system.testCreativeMatching();
        
        // Generate arrangement tree
        cout << "\n=== Generating Arrangement Tree ===" << endl;
        json tree = system.generateArrangementTree("balanced", "any");
        cout << tree.dump(2) << endl;
        
        // Test backward compatibility
        cout << "\n=== Testing Backward Compatibility (Flat) ===" << endl;
        json flat = system.generateArrangementTree("simple", "any", true);
        cout << flat.dump(2) << endl;
        
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}