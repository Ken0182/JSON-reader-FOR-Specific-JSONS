#include "multi_dimensional_pointing_system.h"
#include "json_reader_system.h"
#include <nlohmann/json.hpp>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <unordered_map>
#include <queue>

using json = nlohmann::json;

struct CompatibilityResult {
    float overallScore = 0.0f;
    float semanticScore = 0.0f;
    float technicalScore = 0.0f;
    float musicalRoleScore = 0.0f;
    float layeringScore = 0.0f;
    float idScore = 0.0f;
    std::vector<std::string> strengths;
    std::vector<std::string> issues;
    std::vector<std::string> creative_insights;
    bool is_creative_match = false;
};

struct ArrangementNode {
    std::string root_id;
    std::vector<struct ChildNode> children;
    float overall_compatibility = 0.0f;
    std::string rationale;
};

struct ChildNode {
    std::string id;
    float compat_score = 0.0f;
    std::string rationale;
    std::vector<std::string> shared_properties;
};

class MultiDimensionalPointingSystem {
private:
    JsonReaderSystem* jsonReader;
    std::unordered_map<std::string, json> allEntries;
    std::unordered_map<std::string, std::vector<std::string>> compatibilityGraph;
    
    // 4Z ID parsing and math
    ParsedId parseId(const std::string& id) {
        return jsonReader->parseId(id);
    }
    
    int gcd(int a, int b) {
        while (b != 0) {
            int temp = b;
            b = a % b;
            a = temp;
        }
        return a;
    }
    
    float calculateIdCompatibility(const ParsedId& a, const ParsedId& b, 
                                 std::vector<std::string>& explanations) {
        float idScore = 0.0f;
        
        // Prime GCD compatibility (tuning harmony)
        int tuning_gcd = gcd(a.tuning_prime, b.tuning_prime);
        if (tuning_gcd > 1) {
            idScore += 0.1f;
            explanations.push_back("Prime harmonic match (GCD=" + std::to_string(tuning_gcd) + ")");
        }
        
        // Digit proximity scoring
        int trans_diff = std::abs(a.trans_digit - b.trans_digit);
        if (trans_diff < 10) {
            float proximity_score = 0.05f * (10 - trans_diff) / 10.0f;
            idScore += proximity_score;
            explanations.push_back("Transient proximity ±" + std::to_string(trans_diff) + 
                                 " (score: " + std::to_string(proximity_score) + ")");
        }
        
        int harm_diff = std::abs(a.harm_digit - b.harm_digit);
        if (harm_diff < 15) {
            float proximity_score = 0.04f * (15 - harm_diff) / 15.0f;
            idScore += proximity_score;
            explanations.push_back("Harmonic complexity proximity ±" + std::to_string(harm_diff));
        }
        
        int fx_diff = std::abs(a.fx_digit - b.fx_digit);
        if (fx_diff < 20) {
            float proximity_score = 0.03f * (20 - fx_diff) / 20.0f;
            idScore += proximity_score;
            explanations.push_back("FX complexity proximity ±" + std::to_string(fx_diff));
        }
        
        int damp_diff = std::abs(a.damp_digit - b.damp_digit);
        if (damp_diff < 15) {
            float proximity_score = 0.04f * (15 - damp_diff) / 15.0f;
            idScore += proximity_score;
            explanations.push_back("Dynamic range proximity ±" + std::to_string(damp_diff));
        }
        
        int freq_diff = std::abs(a.freq_digit - b.freq_digit);
        if (freq_diff < 20) {
            float proximity_score = 0.03f * (20 - freq_diff) / 20.0f;
            idScore += proximity_score;
            explanations.push_back("Frequency range proximity ±" + std::to_string(freq_diff));
        }
        
        // Dimensional compatibility
        if (a.dim == b.dim) {
            idScore += 0.02f;
            explanations.push_back("Same dimensional focus");
        }
        
        return std::min(1.0f, idScore);
    }
    
    float calculateSemanticCompatibility(const json& a, const json& b,
                                       std::vector<std::string>& explanations) {
        float score = 0.0f;
        
        // Emotional tag matching
        if (a.contains("emotional") && b.contains("emotional")) {
            auto emotionsA = a["emotional"];
            auto emotionsB = b["emotional"];
            
            if (emotionsA.is_array() && emotionsB.is_array()) {
                std::set<std::string> setA, setB;
                for (const auto& emotion : emotionsA) {
                    if (emotion.is_string()) setA.insert(emotion);
                }
                for (const auto& emotion : emotionsB) {
                    if (emotion.is_string()) setB.insert(emotion);
                }
                
                // Calculate Jaccard similarity
                std::vector<std::string> intersection, union_set;
                std::set_intersection(setA.begin(), setA.end(), setB.begin(), setB.end(),
                                    std::back_inserter(intersection));
                std::set_union(setA.begin(), setA.end(), setB.begin(), setB.end(),
                             std::back_inserter(union_set));
                
                if (!union_set.empty()) {
                    float jaccard = float(intersection.size()) / float(union_set.size());
                    score += jaccard * 0.7f;
                    
                    if (jaccard > 0.3f) {
                        explanations.push_back("Strong emotional alignment (" + 
                                             std::to_string(intersection.size()) + " shared emotions)");
                    }
                }
            }
        }
        
        // Sound generation compatibility
        std::string genA = a.value("soundGeneration", "unknown");
        std::string genB = b.value("soundGeneration", "unknown");
        
        if (genA == genB && genA != "unknown") {
            score += 0.3f;
            explanations.push_back("Same sound generation method: " + genA);
        } else if ((genA == "analog" && genB == "acoustic") || 
                   (genA == "acoustic" && genB == "analog")) {
            score += 0.15f;
            explanations.push_back("Compatible organic sound sources");
        }
        
        return std::min(1.0f, score);
    }
    
    float calculateTechnicalCompatibility(const json& a, const json& b,
                                        std::vector<std::string>& explanations) {
        float score = 0.0f;
        
        // Harmonic content compatibility
        if (a.contains("harmonicContent") && b.contains("harmonicContent")) {
            std::string complexityA = a["harmonicContent"].value("complexity", "unknown");
            std::string complexityB = b["harmonicContent"].value("complexity", "unknown");
            
            if (complexityA == complexityB && complexityA != "unknown") {
                score += 0.4f;
                explanations.push_back("Matching harmonic complexity: " + complexityA);
            } else if ((complexityA == "low" && complexityB == "medium") ||
                       (complexityA == "medium" && complexityB == "low") ||
                       (complexityA == "medium" && complexityB == "high") ||
                       (complexityA == "high" && complexityB == "medium")) {
                score += 0.2f;
                explanations.push_back("Adjacent harmonic complexity levels");
            }
        }
        
        // Transient compatibility
        if (a.contains("transientDetail") && b.contains("transientDetail")) {
            bool enabledA = a["transientDetail"].value("enabled", false);
            bool enabledB = b["transientDetail"].value("enabled", false);
            
            if (enabledA && enabledB) {
                auto intensityA = a["transientDetail"]["intensity"];
                auto intensityB = b["transientDetail"]["intensity"];
                
                if (intensityA.is_array() && intensityB.is_array() && 
                    intensityA.size() >= 2 && intensityB.size() >= 2) {
                    float avgA = (intensityA[0].get<float>() + intensityA[1].get<float>()) / 2.0f;
                    float avgB = (intensityB[0].get<float>() + intensityB[1].get<float>()) / 2.0f;
                    
                    float diff = std::abs(avgA - avgB);
                    if (diff < 0.2f) {
                        score += 0.3f * (1.0f - diff / 0.2f);
                        explanations.push_back("Similar transient characteristics");
                    }
                }
            }
        }
        
        // Tuning system compatibility
        std::string tuningA = a.value("theoryTuning", "unknown");
        std::string tuningB = b.value("theoryTuning", "unknown");
        
        if (tuningA == tuningB && tuningA != "unknown") {
            score += 0.3f;
            explanations.push_back("Same tuning system: " + tuningA);
        }
        
        return std::min(1.0f, score);
    }
    
    float calculateMusicalRoleCompatibility(const json& a, const json& b,
                                          std::vector<std::string>& explanations) {
        float score = 0.0f;
        
        // Extract role from ID type or infer from properties
        std::string roleA = inferRole(a);
        std::string roleB = inferRole(b);
        
        // Role compatibility matrix
        std::unordered_map<std::string, std::vector<std::string>> roleCompatibility = {
            {"lead", {"harmony", "rhythm", "texture"}},
            {"bass", {"rhythm", "harmony", "lead"}},
            {"pad", {"lead", "harmony", "texture"}},
            {"rhythm", {"bass", "lead", "harmony"}},
            {"harmony", {"lead", "bass", "pad"}},
            {"texture", {"pad", "lead", "ambient"}}
        };
        
        if (roleA == roleB) {
            score += 0.2f; // Same role can work but lower score
            explanations.push_back("Same musical role: " + roleA);
        } else {
            auto compatRoles = roleCompatibility.find(roleA);
            if (compatRoles != roleCompatibility.end()) {
                auto& roles = compatRoles->second;
                if (std::find(roles.begin(), roles.end(), roleB) != roles.end()) {
                    score += 0.8f;
                    explanations.push_back("Complementary roles: " + roleA + " + " + roleB);
                }
            }
        }
        
        // FX categories compatibility
        if (a.contains("fxCategories") && b.contains("fxCategories")) {
            auto fxA = a["fxCategories"];
            auto fxB = b["fxCategories"];
            
            if (fxA.is_array() && fxB.is_array()) {
                std::set<std::string> setA, setB;
                for (const auto& fx : fxA) {
                    if (fx.is_string()) setA.insert(fx);
                }
                for (const auto& fx : fxB) {
                    if (fx.is_string()) setB.insert(fx);
                }
                
                std::vector<std::string> intersection;
                std::set_intersection(setA.begin(), setA.end(), setB.begin(), setB.end(),
                                    std::back_inserter(intersection));
                
                if (!intersection.empty()) {
                    float fx_score = float(intersection.size()) / 
                                   float(std::max(setA.size(), setB.size()));
                    score += fx_score * 0.4f;
                    explanations.push_back("Shared FX categories: " + 
                                         std::to_string(intersection.size()));
                }
            }
        }
        
        return std::min(1.0f, score);
    }
    
    float calculateLayeringCompatibility(const json& a, const json& b,
                                       std::vector<std::string>& explanations) {
        float score = 0.0f;
        
        // Frequency range compatibility
        std::string freqA = a.value("frequencyRange", "unknown");
        std::string freqB = b.value("frequencyRange", "unknown");
        
        // Non-overlapping frequency ranges are preferred for layering
        std::unordered_map<std::string, int> freqOrder = {
            {"low", 1}, {"low-mid", 2}, {"mid", 3}, {"mid-high", 4}, {"high", 5}, {"full-spectrum", 0}
        };
        
        if (freqA != "unknown" && freqB != "unknown") {
            if (freqA == "full-spectrum" || freqB == "full-spectrum") {
                score += 0.3f; // Full spectrum can layer with anything
                explanations.push_back("Full spectrum compatibility");
            } else {
                int orderA = freqOrder[freqA];
                int orderB = freqOrder[freqB];
                int diff = std::abs(orderA - orderB);
                
                if (diff >= 2) {
                    score += 0.8f; // Non-overlapping ranges
                    explanations.push_back("Non-overlapping frequency ranges");
                } else if (diff == 1) {
                    score += 0.4f; // Adjacent ranges
                    explanations.push_back("Adjacent frequency ranges");
                } else {
                    score += 0.1f; // Same range, low compatibility
                    explanations.push_back("Overlapping frequency ranges");
                }
            }
        }
        
        // Dynamic range compatibility
        std::string dynA = a.value("dynamicRange", "unknown");
        std::string dynB = b.value("dynamicRange", "unknown");
        
        if (dynA != "unknown" && dynB != "unknown") {
            if ((dynA.find("compressed") != std::string::npos && 
                 dynB.find("expanded") != std::string::npos) ||
                (dynA.find("expanded") != std::string::npos && 
                 dynB.find("compressed") != std::string::npos)) {
                score += 0.6f;
                explanations.push_back("Complementary dynamic ranges");
            } else if (dynA == dynB) {
                score += 0.3f;
                explanations.push_back("Matching dynamic characteristics");
            }
        }
        
        return std::min(1.0f, score);
    }
    
    std::string inferRole(const json& entry) {
        // Check for explicit role indicators
        if (entry.contains("role")) {
            return entry["role"];
        }
        
        // Infer from frequency range
        std::string freq = entry.value("frequencyRange", "");
        if (freq == "low" || freq == "low-mid") return "bass";
        if (freq == "high" || freq == "mid-high") return "lead";
        if (freq == "full-spectrum") return "harmony";
        
        // Infer from type
        std::string type = entry.value("type", "");
        if (type.find("Bass") != std::string::npos || 
            type.find("bass") != std::string::npos) return "bass";
        if (type.find("Lead") != std::string::npos || 
            type.find("lead") != std::string::npos) return "lead";
        if (type.find("Pad") != std::string::npos || 
            type.find("pad") != std::string::npos) return "pad";
        if (type.find("Texture") != std::string::npos || 
            type.find("texture") != std::string::npos) return "texture";
        if (type.find("Arp") != std::string::npos || 
            type.find("arp") != std::string::npos) return "rhythm";
        
        // Default
        return "harmony";
    }

public:
    MultiDimensionalPointingSystem(JsonReaderSystem* reader) : jsonReader(reader) {
        buildCompatibilityGraph();
    }
    
    void buildCompatibilityGraph() {
        // Load all entries from JSON reader
        loadAllEntries();
        
        // Build compatibility relationships
        for (const auto& [idA, entryA] : allEntries) {
            for (const auto& [idB, entryB] : allEntries) {
                if (idA != idB) {
                    CompatibilityResult result = analyzeCompatibility(entryA, entryB);
                    if (result.overallScore > 0.5f) {
                        compatibilityGraph[idA].push_back(idB);
                    }
                }
            }
        }
    }
    
    void loadAllEntries() {
        // Load from all JSON data sources
        const auto& guitarData = jsonReader->getGuitarData();
        const auto& groupData = jsonReader->getGroupData();
        const auto& moodsData = jsonReader->getMoodsData();
        const auto& structureData = jsonReader->getStructureData();
        const auto& synthData = jsonReader->getSynthData();
        
        // Extract entries with IDs
        if (guitarData.contains("groups")) {
            for (const auto& [key, entry] : guitarData["groups"].items()) {
                if (entry.contains("id")) {
                    allEntries[entry["id"]] = entry;
                }
            }
        }
        
        if (groupData.contains("groups")) {
            for (const auto& [key, entry] : groupData["groups"].items()) {
                if (entry.contains("id")) {
                    allEntries[entry["id"]] = entry;
                }
            }
        }
        
        // Continue for other data sources...
    }
    
    CompatibilityResult analyzeCompatibility(const json& a, const json& b) {
        CompatibilityResult result;
        
        // Parse IDs for mathematical compatibility
        ParsedId parsedA, parsedB;
        bool hasValidIds = false;
        
        if (a.contains("id") && b.contains("id")) {
            parsedA = parseId(a["id"]);
            parsedB = parseId(b["id"]);
            hasValidIds = true;
        }
        
        // Calculate individual dimension scores
        result.semanticScore = calculateSemanticCompatibility(a, b, result.strengths);
        result.technicalScore = calculateTechnicalCompatibility(a, b, result.strengths);
        result.musicalRoleScore = calculateMusicalRoleCompatibility(a, b, result.strengths);
        result.layeringScore = calculateLayeringCompatibility(a, b, result.strengths);
        
        // Calculate ID compatibility score
        if (hasValidIds) {
            std::vector<std::string> idExplanations;
            result.idScore = calculateIdCompatibility(parsedA, parsedB, idExplanations);
            result.strengths.insert(result.strengths.end(), 
                                  idExplanations.begin(), idExplanations.end());
        }
        
        // Weighted overall score
        result.overallScore = (result.semanticScore * 0.25f +
                              result.technicalScore * 0.25f +
                              result.musicalRoleScore * 0.25f +
                              result.layeringScore * 0.20f +
                              result.idScore * 0.15f); // ID weight of 15%
        
        // Creative compatibility logic
        if (result.idScore > 0.3f && 
            (result.semanticScore > 0.8f || result.technicalScore > 0.8f) &&
            (result.musicalRoleScore < 0.6f || result.layeringScore < 0.6f)) {
            result.overallScore += 0.05f; // Creative boost
            result.is_creative_match = true;
            result.creative_insights.push_back("Unexpected synergy: High property compatibility despite role mismatch");
        }
        
        // Add ID math insights
        if (hasValidIds && result.idScore > 0.15f) {
            result.strengths.push_back("Strong ID mathematical compatibility: " + 
                                     std::to_string(result.idScore));
        }
        
        return result;
    }
    
    ArrangementNode generateArrangement(const std::string& rootId, int maxChildren = 5) {
        ArrangementNode arrangement;
        arrangement.root_id = rootId;
        
        if (allEntries.find(rootId) == allEntries.end()) {
            return arrangement; // Empty if root not found
        }
        
        const json& rootEntry = allEntries[rootId];
        std::priority_queue<std::pair<float, std::string>> candidates;
        
        // Find compatible entries
        for (const auto& [id, entry] : allEntries) {
            if (id != rootId) {
                CompatibilityResult compat = analyzeCompatibility(rootEntry, entry);
                
                // Use ID pre-filtering for efficiency
                ParsedId rootParsed = parseId(rootId);
                ParsedId candidateParsed = parseId(id);
                
                // Quick ID compatibility check
                bool idCompatible = (gcd(rootParsed.tuning_prime, candidateParsed.tuning_prime) > 1) ||
                                   (std::abs(rootParsed.trans_digit - candidateParsed.trans_digit) < 10);
                
                if (compat.overallScore > 0.4f || (idCompatible && compat.overallScore > 0.3f)) {
                    candidates.push({compat.overallScore, id});
                }
            }
        }
        
        // Select top candidates with 30% creative bias
        int creativeCandidates = std::max(1, maxChildren * 30 / 100);
        int standardCandidates = maxChildren - creativeCandidates;
        
        // Add standard high-compatibility matches
        for (int i = 0; i < standardCandidates && !candidates.empty(); i++) {
            auto [score, id] = candidates.top();
            candidates.pop();
            
            CompatibilityResult finalCompat = analyzeCompatibility(rootEntry, allEntries[id]);
            
            ChildNode child;
            child.id = id;
            child.compat_score = score;
            child.rationale = buildRationale(finalCompat);
            child.shared_properties = extractSharedProperties(rootEntry, allEntries[id]);
            
            arrangement.children.push_back(child);
        }
        
        // Add creative matches (high ID compatibility with explanation)
        std::vector<std::pair<float, std::string>> creativeMatches;
        for (const auto& [id, entry] : allEntries) {
            if (id != rootId) {
                CompatibilityResult compat = analyzeCompatibility(rootEntry, entry);
                if (compat.is_creative_match || compat.idScore > 0.3f) {
                    creativeMatches.push_back({compat.idScore, id});
                }
            }
        }
        
        std::sort(creativeMatches.rbegin(), creativeMatches.rend());
        
        for (int i = 0; i < creativeCandidates && i < creativeMatches.size(); i++) {
            auto [idScore, id] = creativeMatches[i];
            CompatibilityResult finalCompat = analyzeCompatibility(rootEntry, allEntries[id]);
            
            ChildNode child;
            child.id = id;
            child.compat_score = finalCompat.overallScore;
            child.rationale = "Creative: " + buildRationale(finalCompat);
            child.shared_properties = extractSharedProperties(rootEntry, allEntries[id]);
            
            arrangement.children.push_back(child);
        }
        
        // Calculate overall arrangement compatibility
        if (!arrangement.children.empty()) {
            float avgCompat = 0.0f;
            for (const auto& child : arrangement.children) {
                avgCompat += child.compat_score;
            }
            arrangement.overall_compatibility = avgCompat / arrangement.children.size();
            arrangement.rationale = "Multi-dimensional arrangement with " + 
                                   std::to_string(arrangement.children.size()) + " compatible elements";
        }
        
        return arrangement;
    }
    
private:
    std::string buildRationale(const CompatibilityResult& result) {
        std::string rationale;
        
        if (!result.strengths.empty()) {
            rationale += "Strengths: ";
            for (size_t i = 0; i < std::min(size_t(3), result.strengths.size()); i++) {
                if (i > 0) rationale += ", ";
                rationale += result.strengths[i];
            }
        }
        
        if (result.is_creative_match) {
            rationale += " [Creative Match]";
        }
        
        return rationale;
    }
    
    std::vector<std::string> extractSharedProperties(const json& a, const json& b) {
        std::vector<std::string> shared;
        
        // Check shared FX categories
        if (a.contains("fxCategories") && b.contains("fxCategories")) {
            auto fxA = a["fxCategories"];
            auto fxB = b["fxCategories"];
            if (fxA.is_array() && fxB.is_array()) {
                std::set<std::string> setA, setB;
                for (const auto& fx : fxA) {
                    if (fx.is_string()) setA.insert(fx);
                }
                for (const auto& fx : fxB) {
                    if (fx.is_string()) setB.insert(fx);
                }
                
                std::vector<std::string> intersection;
                std::set_intersection(setA.begin(), setA.end(), setB.begin(), setB.end(),
                                    std::back_inserter(intersection));
                
                for (const auto& fx : intersection) {
                    shared.push_back("fx:" + fx);
                }
            }
        }
        
        // Check shared properties
        std::vector<std::string> propertiesToCheck = {
            "soundGeneration", "theoryTuning", "dynamicRange", "frequencyRange"
        };
        
        for (const auto& prop : propertiesToCheck) {
            if (a.contains(prop) && b.contains(prop) && a[prop] == b[prop]) {
                shared.push_back(prop + ":" + a[prop].get<std::string>());
            }
        }
        
        return shared;
    }
};