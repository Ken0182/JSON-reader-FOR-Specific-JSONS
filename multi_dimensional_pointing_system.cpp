#include "json.hpp"
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
        
        // Buffer size compatibility
        totalChecks++;
        int bufferOverlapStart = max(a.techSpecs.bufferSizeMin, b.techSpecs.bufferSizeMin);
        int bufferOverlapEnd = min(a.techSpecs.bufferSizeMax, b.techSpecs.bufferSizeMax);
        if (bufferOverlapEnd >= bufferOverlapStart) {
            score += 1.0f;
            result.strengths.push_back("Compatible buffer sizes");
        } else {
            result.issues.push_back("Incompatible buffer size ranges");
        }
        
        // Effect compatibility check
        totalChecks++;
        bool effectConflict = false;
        for (const string& requiredEffect : a.techSpecs.requiredEffects) {
            for (const string& incompatibleEffect : b.techSpecs.incompatibleEffects) {
                if (requiredEffect == incompatibleEffect) {
                    result.issues.push_back("Effect conflict: " + a.name + " requires " + requiredEffect + 
                                          " but " + b.name + " is incompatible with it");
                    effectConflict = true;
                }
            }
        }
        if (!effectConflict) {
            score += 1.0f;
            result.strengths.push_back("No effect conflicts detected");
        }
        
        // Plugin format compatibility
        totalChecks++;
        bool formatMatch = false;
        for (const string& formatA : a.techSpecs.supportedFormats) {
            for (const string& formatB : b.techSpecs.supportedFormats) {
                if (formatA == formatB) {
                    formatMatch = true;
                    result.strengths.push_back("Compatible plugin format: " + formatA);
                    break;
                }
            }
            if (formatMatch) break;
        }
        if (formatMatch) {
            score += 1.0f;
        } else {
            result.issues.push_back("No compatible plugin formats");
        }
        
        result.compatibilityScore = score / totalChecks;
        result.isCompatible = result.compatibilityScore >= 0.7f && result.issues.empty();
        
        return result;
    }
    
    bool validateChainCompatibility(const vector<EnhancedConfigEntry>& chain) {
        for (size_t i = 0; i < chain.size() - 1; ++i) {
            auto result = checkTechnicalCompatibility(chain[i], chain[i + 1]);
            if (!result.isCompatible) {
                return false;
            }
        }
        return true;
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
    
    vector<EnhancedConfigEntry> suggestRoleBasedChain(const EnhancedConfigEntry& anchor, 
                                                     const vector<EnhancedConfigEntry>& candidates) {
        vector<EnhancedConfigEntry> suggestions;
        
        for (const auto& candidate : candidates) {
            float compatibility = calculateMusicalRoleCompatibility(anchor, candidate);
            if (compatibility >= 0.6f) {
                suggestions.push_back(candidate);
            }
        }
        
        // Sort by compatibility score
        sort(suggestions.begin(), suggestions.end(), 
             [&](const EnhancedConfigEntry& a, const EnhancedConfigEntry& b) {
                 return calculateMusicalRoleCompatibility(anchor, a) > 
                        calculateMusicalRoleCompatibility(anchor, b);
             });
        
        return suggestions;
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
    
    struct LayeringConfiguration {
        vector<EnhancedConfigEntry> foregroundInstruments;
        vector<EnhancedConfigEntry> midgroundInstruments;
        vector<EnhancedConfigEntry> backgroundInstruments;
        map<string, vector<EnhancedConfigEntry>> arrangementSections;
        float overallBalance = 0.0f;
    };
    
    LayeringConfiguration generateOptimalLayering(const vector<EnhancedConfigEntry>& instruments) {
        LayeringConfiguration config;
        
        // Distribute instruments by preferred layer
        for (const auto& instrument : instruments) {
            string layer = instrument.layeringInfo.preferredLayer;
            if (layer == "foreground") {
                config.foregroundInstruments.push_back(instrument);
            } else if (layer == "midground") {
                config.midgroundInstruments.push_back(instrument);
            } else if (layer == "background") {
                config.backgroundInstruments.push_back(instrument);
            }
        }
        
        // Group by arrangement sections
        for (const auto& instrument : instruments) {
            string section = instrument.layeringInfo.arrangementPosition;
            config.arrangementSections[section].push_back(instrument);
        }
        
        // Calculate balance score
        float layerBalance = 1.0f;
        if (!config.foregroundInstruments.empty() && !config.backgroundInstruments.empty()) {
            layerBalance = min(layerBalance, 1.0f - abs((float)config.foregroundInstruments.size() - 
                                                       (float)config.backgroundInstruments.size()) / 
                                                  (float)instruments.size());
        }
        
        config.overallBalance = layerBalance;
        
        return config;
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
    
public:
    MultiDimensionalPointingSystem() {
        loadConfigDatabase();
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
        } else if (nameLower.find("chord") != string::npos) {
            entry.musicalRole.primaryRole = "chord";
            entry.musicalRole.prominence = 0.5f;
            entry.musicalRole.isHarmonic = true;
        } else if (entry.category == "guitar") {
            entry.musicalRole.primaryRole = "lead";
            entry.musicalRole.prominence = 0.8f;
        } else {
            entry.musicalRole.primaryRole = "pad";
            entry.musicalRole.prominence = 0.4f;
        }
        
        // Set musical context based on topological metadata
        if (config.contains("topologicalMetadata")) {
            const auto& topo = config["topologicalMetadata"];
            if (topo.contains("manifold_position")) {
                string position = topo["manifold_position"].get<string>();
                if (position.find("intro") != string::npos) {
                    entry.musicalRole.musicalContext = "intro";
                } else if (position.find("verse") != string::npos) {
                    entry.musicalRole.musicalContext = "verse";
                } else if (position.find("chorus") != string::npos) {
                    entry.musicalRole.musicalContext = "chorus";
                } else if (position.find("bridge") != string::npos) {
                    entry.musicalRole.musicalContext = "bridge";
                } else if (position.find("outro") != string::npos) {
                    entry.musicalRole.musicalContext = "outro";
                }
            }
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
        
        // Set arrangement position from musical context
        entry.layeringInfo.arrangementPosition = entry.musicalRole.musicalContext;
        
        // Set mix priority same as prominence
        entry.layeringInfo.mixPriority = entry.musicalRole.prominence;
    }
    
    void setCompatibilityInfo(EnhancedConfigEntry& entry, const json& config) {
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
        
        // Set incompatibilities
        if (entry.musicalRole.prominence >= 0.8f) {
            // High prominence instruments shouldn't both be foreground
            entry.incompatibleWith.push_back("high_prominence");
        }
    }

public:
    /**
     * Multi-dimensional compatibility analysis
     * Combines all four pointing dimensions
     */
    struct MultiDimensionalResult {
        float overallScore = 0.0f;
        float semanticScore = 0.0f;
        float technicalScore = 0.0f;
        float musicalRoleScore = 0.0f;
        float layeringScore = 0.0f;
        
        bool isRecommended = false;
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
        
        // Calculate weighted overall score
        result.overallScore = (0.2f * result.semanticScore +     // 20% semantic
                              0.3f * result.technicalScore +     // 30% technical
                              0.3f * result.musicalRoleScore +   // 30% musical role
                              0.2f * result.layeringScore);      // 20% layering
        
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
        
        // Find anchor configuration
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
     * Generate a complete musical arrangement
     */
    struct MusicalArrangement {
        EnhancedConfigEntry lead;
        EnhancedConfigEntry bass;
        vector<EnhancedConfigEntry> harmony;
        vector<EnhancedConfigEntry> rhythm;
        vector<EnhancedConfigEntry> effects;
        float overallCompatibility = 0.0f;
        vector<string> arrangementNotes;
    };
    
    MusicalArrangement generateArrangement(const string& style = "balanced", 
                                          const string& context = "any") {
        MusicalArrangement arrangement;
        
        // Find lead instrument
        auto leadCandidates = findByRole("lead");
        if (!leadCandidates.empty()) {
            arrangement.lead = leadCandidates[0];
        }
        
        // Find bass instrument
        auto bassCandidates = findByRole("bass");
        if (!bassCandidates.empty()) {
            arrangement.bass = bassCandidates[0];
        }
        
        // Find harmony instruments
        auto harmonyCandidates = findByRole("pad");
        for (size_t i = 0; i < min((size_t)2, harmonyCandidates.size()); ++i) {
            arrangement.harmony.push_back(harmonyCandidates[i]);
        }
        
        // Add effects
        auto effectCandidates = findByCategory("effect");
        for (size_t i = 0; i < min((size_t)3, effectCandidates.size()); ++i) {
            arrangement.effects.push_back(effectCandidates[i]);
        }
        
        // Calculate overall compatibility
        float totalScore = 0.0f;
        int pairCount = 0;
        
        vector<EnhancedConfigEntry> allInstruments = {arrangement.lead, arrangement.bass};
        allInstruments.insert(allInstruments.end(), arrangement.harmony.begin(), arrangement.harmony.end());
        
        for (size_t i = 0; i < allInstruments.size(); ++i) {
            for (size_t j = i + 1; j < allInstruments.size(); ++j) {
                auto result = analyzeCompatibility(allInstruments[i], allInstruments[j]);
                totalScore += result.overallScore;
                pairCount++;
            }
        }
        
        if (pairCount > 0) {
            arrangement.overallCompatibility = totalScore / pairCount;
        }
        
        // Generate arrangement notes
        arrangement.arrangementNotes.push_back("Lead: " + arrangement.lead.name);
        arrangement.arrangementNotes.push_back("Bass: " + arrangement.bass.name);
        for (const auto& harm : arrangement.harmony) {
            arrangement.arrangementNotes.push_back("Harmony: " + harm.name);
        }
        arrangement.arrangementNotes.push_back("Overall compatibility: " + 
                                              to_string(arrangement.overallCompatibility));
        
        return arrangement;
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
    
    vector<EnhancedConfigEntry> findByCategory(const string& category) {
        vector<EnhancedConfigEntry> results;
        for (const auto& entry : configDatabase) {
            if (entry.category == category) {
                results.push_back(entry);
            }
        }
        return results;
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
        
        cout << "=========================================================" << endl;
    }
    
    /**
     * Export preset with full metadata
     */
    json exportPresetWithMetadata(const vector<string>& configIds) {
        json preset = json::object();
        preset["metadata"] = {
            {"version", "1.0"},
            {"created", chrono::duration_cast<chrono::seconds>(
                chrono::system_clock::now().time_since_epoch()).count()},
            {"multidimensional_pointing", true}
        };
        
        json instruments = json::array();
        
        for (const string& id : configIds) {
            auto entryIt = find_if(configDatabase.begin(), configDatabase.end(),
                                  [&](const EnhancedConfigEntry& entry) { return entry.id == id; });
            
            if (entryIt != configDatabase.end()) {
                json instrumentData = json::object();
                instrumentData["id"] = entryIt->id;
                instrumentData["name"] = entryIt->name;
                instrumentData["category"] = entryIt->category;
                instrumentData["config"] = entryIt->configData;
                
                // Add multi-dimensional metadata
                instrumentData["semantic_tags"] = entryIt->semanticTags;
                instrumentData["musical_role"] = {
                    {"primary_role", entryIt->musicalRole.primaryRole},
                    {"musical_context", entryIt->musicalRole.musicalContext},
                    {"prominence", entryIt->musicalRole.prominence},
                    {"tonal_character", entryIt->musicalRole.tonalCharacter}
                };
                instrumentData["layering_info"] = {
                    {"preferred_layer", entryIt->layeringInfo.preferredLayer},
                    {"frequency_range", entryIt->layeringInfo.frequencyRange},
                    {"stereo_width", entryIt->layeringInfo.stereoWidth},
                    {"mix_priority", entryIt->layeringInfo.mixPriority}
                };
                instrumentData["technical_specs"] = {
                    {"sample_rate", entryIt->techSpecs.sampleRate},
                    {"bit_depth", entryIt->techSpecs.bitDepth},
                    {"envelope_type", entryIt->techSpecs.envelopeType},
                    {"polyphony_limit", entryIt->techSpecs.polyphonyLimit}
                };
                
                instruments.push_back(instrumentData);
            }
        }
        
        preset["instruments"] = instruments;
        
        // Add compatibility analysis
        json compatibilityMatrix = json::object();
        for (size_t i = 0; i < configIds.size(); ++i) {
            for (size_t j = i + 1; j < configIds.size(); ++j) {
                auto entryA = find_if(configDatabase.begin(), configDatabase.end(),
                                     [&](const EnhancedConfigEntry& entry) { return entry.id == configIds[i]; });
                auto entryB = find_if(configDatabase.begin(), configDatabase.end(),
                                     [&](const EnhancedConfigEntry& entry) { return entry.id == configIds[j]; });
                
                if (entryA != configDatabase.end() && entryB != configDatabase.end()) {
                    auto compatibility = analyzeCompatibility(*entryA, *entryB);
                    string pairKey = configIds[i] + "_" + configIds[j];
                    compatibilityMatrix[pairKey] = {
                        {"overall_score", compatibility.overallScore},
                        {"is_recommended", compatibility.isRecommended},
                        {"semantic_score", compatibility.semanticScore},
                        {"technical_score", compatibility.technicalScore},
                        {"musical_role_score", compatibility.musicalRoleScore},
                        {"layering_score", compatibility.layeringScore}
                    };
                }
            }
        }
        preset["compatibility_analysis"] = compatibilityMatrix;
        
        return preset;
    }
};

// Interactive demo and testing
void runInteractiveDemo() {
    cout << "=== MULTI-DIMENSIONAL POINTING SYSTEM DEMO ===" << endl;
    
    MultiDimensionalPointingSystem system;
    system.printSystemStatistics();
    
    // Find compatible configurations for a lead instrument
    cout << "\n=== Finding Compatible Configurations ===" << endl;
    auto compatibleResults = system.findCompatibleConfigurations("Lead_Bright_Energetic", 5);
    
    cout << "Compatible with Lead_Bright_Energetic:" << endl;
    for (const auto& [config, result] : compatibleResults) {
        cout << "- " << config.name << " (Score: " << fixed << setprecision(2) 
             << result.overallScore << ")" << endl;
        cout << "  Role: " << config.musicalRole.primaryRole 
             << ", Layer: " << config.layeringInfo.preferredLayer << endl;
        cout << "  Strengths: ";
        for (size_t i = 0; i < min((size_t)2, result.strengths.size()); ++i) {
            if (i > 0) cout << ", ";
            cout << result.strengths[i];
        }
        cout << endl << endl;
    }
    
    // Generate a complete musical arrangement
    cout << "\n=== Generating Musical Arrangement ===" << endl;
    auto arrangement = system.generateArrangement("balanced", "any");
    
    cout << "Generated arrangement:" << endl;
    for (const string& note : arrangement.arrangementNotes) {
        cout << "- " << note << endl;
    }
    
    // Export preset with metadata
    cout << "\n=== Exporting Preset with Metadata ===" << endl;
    vector<string> presetIds = {arrangement.lead.id, arrangement.bass.id};
    if (!arrangement.harmony.empty()) {
        presetIds.push_back(arrangement.harmony[0].id);
    }
    
    json preset = system.exportPresetWithMetadata(presetIds);
    
    ofstream presetFile("multi_dimensional_preset.json");
    presetFile << preset.dump(2);
    presetFile.close();
    
    cout << "Preset exported to multi_dimensional_preset.json" << endl;
    cout << "Preset contains " << preset["instruments"].size() << " instruments with full metadata." << endl;
}

int main() {
    cout << "Multi-Dimensional Pointing System - Intelligent Configuration Assembly" << endl;
    cout << "=================================================================" << endl;
    
    try {
        runInteractiveDemo();
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}