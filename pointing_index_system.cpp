#include "pointing_index_system.h"
#include "json_reader_system.h"
#include "multi_dimensional_pointing_system.h"
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

using json = nlohmann::json;

struct IndexedEntry {
    std::string id;
    json entry;
    std::vector<float> embedding;
    std::unordered_map<std::string, float> dynamicVector;
    ParsedId parsedId;
    float vectorMagnitude = 0.0f;
};

struct SearchResult {
    IndexedEntry entry;
    float finalScore = 0.0f;
    float semanticScore = 0.0f;
    float vectorScore = 0.0f;
    float idProximityScore = 0.0f;
    std::vector<std::string> matchReasons;
    bool isCreativeMatch = false;
};

class PointingIndexSystem {
private:
    JsonReaderSystem* jsonReader;
    MultiDimensionalPointingSystem* pointingSystem;
    
    // Dynamic registry for all properties
    std::unordered_map<std::string, float> globalRegistry;
    std::vector<std::string> registryKeys; // Ordered keys for consistent vectorization
    
    // Indexed data structures
    std::vector<IndexedEntry> allEntries;
    std::unordered_map<std::string, std::vector<size_t>> textIndex; // property_value -> entry indices
    std::unordered_map<std::string, std::vector<size_t>> categoryIndex; // category -> entry indices
    std::unordered_map<std::string, size_t> idToIndex; // id -> entry index
    
    // Pre-computed similarity matrices for fast lookup
    std::vector<std::vector<float>> precomputedSimilarity;
    
    void initializeRegistry() {
        // Core properties with weights (ordered by importance)
        registryKeys = {
            "harmonicRichness",      // 0.25 weight - perception importance
            "transientSharpness",    // 0.20 weight - percussive characteristics  
            "fxComplexity",          // 0.15 weight - processing complexity
            "frequencyFocus",        // 0.20 weight - layering importance
            "dynamicCompression",    // 0.20 weight - sustain characteristics
            "tuningStability",       // 0.10 weight - dissonance factor
            "soundGenMethod",        // 0.15 weight - semantic grouping
            "spectralDensity",       // 0.15 weight - harmonic spread
            "temporalEvolution",     // 0.10 weight - time-based changes
            "spatialWidth",          // 0.08 weight - stereo characteristics
            "energyLevel",           // 0.12 weight - overall intensity
            "tonalWarmth"            // 0.10 weight - timbral character
        };
        
        // Initialize global registry with neutral values
        for (const auto& key : registryKeys) {
            globalRegistry[key] = 0.5f; // Neutral starting point
        }
    }
    
    void extractPropertyVector(const json& entry, std::unordered_map<std::string, float>& vector) {
        // Harmonic richness from harmonic content
        if (entry.contains("harmonicContent")) {
            auto complexity = entry["harmonicContent"].value("complexity", "unknown");
            if (complexity == "low") vector["harmonicRichness"] = 0.25f;
            else if (complexity == "medium" || complexity == "med") vector["harmonicRichness"] = 0.5f;
            else if (complexity == "high") vector["harmonicRichness"] = 0.75f;
            else if (complexity == "very high") vector["harmonicRichness"] = 0.9f;
            else if (complexity == "extreme") vector["harmonicRichness"] = 1.0f;
            else {
                // Infer from overtones array
                if (entry["harmonicContent"].contains("overtones")) {
                    auto overtones = entry["harmonicContent"]["overtones"];
                    if (overtones.is_array()) {
                        float richness = std::min(1.0f, float(overtones.size()) / 8.0f);
                        vector["harmonicRichness"] = richness;
                    }
                }
            }
        }
        
        // Transient sharpness from transient detail and attack times
        if (entry.contains("transientDetail") && entry["transientDetail"].contains("intensity")) {
            auto intensity = entry["transientDetail"]["intensity"];
            if (intensity.is_array() && intensity.size() >= 2) {
                float avgIntensity = (intensity[0].get<float>() + intensity[1].get<float>()) / 2.0f;
                vector["transientSharpness"] = avgIntensity;
            }
        } else if (entry.contains("envelope") && entry["envelope"].contains("attack")) {
            // Infer from attack time (shorter = sharper)
            auto attack = entry["envelope"]["attack"];
            if (attack.is_array() && attack.size() >= 2) {
                float avgAttack = (attack[0].get<float>() + attack[1].get<float>()) / 2.0f;
                // Log scale: very short attack = high sharpness
                vector["transientSharpness"] = std::max(0.0f, 1.0f - std::log10(avgAttack * 1000 + 1) / 4.0f);
            }
        }
        
        // FX complexity from FX categories and enabled effects
        float fxComplexity = 0.0f;
        if (entry.contains("fxCategories") && entry["fxCategories"].is_array()) {
            fxComplexity = std::min(1.0f, float(entry["fxCategories"].size()) / 5.0f);
        } else if (entry.contains("fx") && entry["fx"].is_object()) {
            int enabledCount = 0;
            for (const auto& [key, fx] : entry["fx"].items()) {
                if (fx.is_object() && fx.value("enabled", false)) {
                    enabledCount++;
                }
            }
            fxComplexity = std::min(1.0f, float(enabledCount) / 5.0f);
        }
        vector["fxComplexity"] = fxComplexity;
        
        // Frequency focus from frequency range
        std::string freqRange = entry.value("frequencyRange", "mid");
        if (freqRange == "low" || freqRange == "low-mid") vector["frequencyFocus"] = 0.2f;
        else if (freqRange == "mid" || freqRange == "mid-high") vector["frequencyFocus"] = 0.5f;
        else if (freqRange == "high") vector["frequencyFocus"] = 0.8f;
        else if (freqRange == "full-spectrum" || freqRange == "full") vector["frequencyFocus"] = 1.0f;
        else vector["frequencyFocus"] = 0.5f; // default mid
        
        // Dynamic compression from dynamic range
        std::string dynRange = entry.value("dynamicRange", "moderate");
        if (dynRange.find("compressed") != std::string::npos) vector["dynamicCompression"] = 0.8f;
        else if (dynRange.find("expanded") != std::string::npos) vector["dynamicCompression"] = 0.2f;
        else if (dynRange.find("moderate") != std::string::npos) vector["dynamicCompression"] = 0.5f;
        else vector["dynamicCompression"] = 0.5f; // default
        
        // Tuning stability from theory tuning
        std::string tuning = entry.value("theoryTuning", "equal");
        if (tuning == "equal") vector["tuningStability"] = 1.0f;
        else if (tuning == "just" || tuning == "just_intonation") vector["tuningStability"] = 0.9f;
        else if (tuning == "microtonal" || tuning == "micro") vector["tuningStability"] = 0.3f;
        else if (tuning == "atonal") vector["tuningStability"] = 0.1f;
        else vector["tuningStability"] = 0.7f; // neutral
        
        // Sound generation method
        std::string soundGen = entry.value("soundGeneration", "digital");
        if (soundGen == "acoustic") vector["soundGenMethod"] = 0.2f;
        else if (soundGen == "analog") vector["soundGenMethod"] = 0.4f;
        else if (soundGen == "digital") vector["soundGenMethod"] = 0.6f;
        else if (soundGen == "sample-based") vector["soundGenMethod"] = 0.8f;
        else if (soundGen == "hybrid") vector["soundGenMethod"] = 1.0f;
        else vector["soundGenMethod"] = 0.6f; // default digital
        
        // Spectral density from harmonic content and filter characteristics
        float spectralDensity = vector.count("harmonicRichness") ? vector["harmonicRichness"] : 0.5f;
        if (entry.contains("filter") && entry["filter"].contains("resonance")) {
            auto resonance = entry["filter"]["resonance"];
            if (resonance.is_array() && resonance.size() >= 2) {
                float avgResonance = (resonance[0].get<float>() + resonance[1].get<float>()) / 2.0f;
                spectralDensity = (spectralDensity + avgResonance) / 2.0f;
            }
        }
        vector["spectralDensity"] = spectralDensity;
        
        // Temporal evolution from envelope characteristics
        float temporalEvolution = 0.5f;
        if (entry.contains("envelope")) {
            const auto& env = entry["envelope"];
            if (env.contains("attack") && env.contains("release")) {
                // Longer envelopes = more evolution
                float totalTime = 0.0f;
                if (env["attack"].is_array()) {
                    totalTime += (env["attack"][0].get<float>() + env["attack"][1].get<float>()) / 2.0f;
                }
                if (env["release"].is_array()) {
                    totalTime += (env["release"][0].get<float>() + env["release"][1].get<float>()) / 2.0f;
                }
                temporalEvolution = std::min(1.0f, totalTime / 10.0f); // Normalize to 10 seconds max
            }
        }
        vector["temporalEvolution"] = temporalEvolution;
        
        // Spatial width (inferred from instrument type and FX)
        float spatialWidth = 0.5f;
        if (entry.contains("fx")) {
            if (entry["fx"].is_object()) {
                if (entry["fx"].contains("reverb") && entry["fx"]["reverb"].value("enabled", false)) {
                    spatialWidth += 0.3f;
                }
                if (entry["fx"].contains("delay") && entry["fx"]["delay"].value("enabled", false)) {
                    spatialWidth += 0.2f;
                }
                if (entry["fx"].contains("chorus") && entry["fx"]["chorus"].value("enabled", false)) {
                    spatialWidth += 0.2f;
                }
            }
        }
        vector["spatialWidth"] = std::min(1.0f, spatialWidth);
        
        // Energy level from transient sharpness and FX complexity
        float energyLevel = (vector.count("transientSharpness") ? vector["transientSharpness"] : 0.5f) * 0.6f +
                           (vector.count("fxComplexity") ? vector["fxComplexity"] : 0.5f) * 0.4f;
        vector["energyLevel"] = energyLevel;
        
        // Tonal warmth (inferred from various characteristics)
        float tonalWarmth = 0.5f;
        if (entry.contains("emotional") && entry["emotional"].is_array()) {
            for (const auto& emotion : entry["emotional"]) {
                if (emotion.is_string()) {
                    std::string emotionStr = emotion;
                    if (emotionStr == "warm" || emotionStr == "soft") tonalWarmth += 0.3f;
                    else if (emotionStr == "bright" || emotionStr == "sharp") tonalWarmth -= 0.2f;
                    else if (emotionStr == "dark" || emotionStr == "deep") tonalWarmth += 0.1f;
                }
            }
        }
        vector["tonalWarmth"] = std::max(0.0f, std::min(1.0f, tonalWarmth));
        
        // Fill any missing values with neutral defaults
        for (const auto& key : registryKeys) {
            if (vector.find(key) == vector.end()) {
                vector[key] = 0.5f; // Neutral default
            }
        }
    }
    
    void buildTextIndex() {
        for (size_t i = 0; i < allEntries.size(); i++) {
            const auto& entry = allEntries[i].entry;
            
            // Index FX categories
            if (entry.contains("fxCategories") && entry["fxCategories"].is_array()) {
                for (const auto& fx : entry["fxCategories"]) {
                    if (fx.is_string()) {
                        textIndex["fx:" + fx.get<std::string>()].push_back(i);
                    }
                }
            }
            
            // Index sound generation
            if (entry.contains("soundGeneration")) {
                textIndex["soundGen:" + entry["soundGeneration"].get<std::string>()].push_back(i);
            }
            
            // Index theory tuning
            if (entry.contains("theoryTuning")) {
                textIndex["tuning:" + entry["theoryTuning"].get<std::string>()].push_back(i);
            }
            
            // Index frequency range
            if (entry.contains("frequencyRange")) {
                textIndex["freq:" + entry["frequencyRange"].get<std::string>()].push_back(i);
            }
            
            // Index dynamic range
            if (entry.contains("dynamicRange")) {
                textIndex["dynamic:" + entry["dynamicRange"].get<std::string>()].push_back(i);
            }
            
            // Index emotional tags
            if (entry.contains("emotional") && entry["emotional"].is_array()) {
                for (const auto& emotion : entry["emotional"]) {
                    if (emotion.is_string()) {
                        textIndex["emotion:" + emotion.get<std::string>()].push_back(i);
                    }
                }
            }
            
            // Index type/category
            if (entry.contains("type")) {
                categoryIndex[entry["type"].get<std::string>()].push_back(i);
            }
        }
    }
    
    float calculateVectorSimilarity(const std::vector<float>& a, const std::vector<float>& b) {
        if (a.size() != b.size()) return 0.0f;
        
        float dotProduct = 0.0f;
        float normA = 0.0f;
        float normB = 0.0f;
        
        for (size_t i = 0; i < a.size(); i++) {
            dotProduct += a[i] * b[i];
            normA += a[i] * a[i];
            normB += b[i] * b[i];
        }
        
        if (normA == 0.0f || normB == 0.0f) return 0.0f;
        
        return dotProduct / (std::sqrt(normA) * std::sqrt(normB));
    }
    
    float calculateIdProximity(const ParsedId& a, const ParsedId& b, 
                             std::vector<std::string>& explanations) {
        float proximityScore = 0.0f;
        
        // Transient proximity (most important for creative matching)
        int transDiff = std::abs(a.trans_digit - b.trans_digit);
        if (transDiff < 10) {
            float score = 0.4f * (10 - transDiff) / 10.0f;
            proximityScore += score;
            explanations.push_back("Transient proximity ±" + std::to_string(transDiff));
        }
        
        // Harmonic proximity
        int harmDiff = std::abs(a.harm_digit - b.harm_digit);
        if (harmDiff < 15) {
            float score = 0.3f * (15 - harmDiff) / 15.0f;
            proximityScore += score;
            explanations.push_back("Harmonic proximity ±" + std::to_string(harmDiff));
        }
        
        // FX proximity
        int fxDiff = std::abs(a.fx_digit - b.fx_digit);
        if (fxDiff < 20) {
            float score = 0.2f * (20 - fxDiff) / 20.0f;
            proximityScore += score;
            explanations.push_back("FX complexity proximity ±" + std::to_string(fxDiff));
        }
        
        // Prime compatibility for tuning
        if (jsonReader) {
            int gcd_val = gcd(a.tuning_prime, b.tuning_prime);
            if (gcd_val > 1) {
                proximityScore += 0.1f;
                explanations.push_back("Prime tuning compatibility (GCD=" + std::to_string(gcd_val) + ")");
            }
        }
        
        return std::min(1.0f, proximityScore);
    }
    
    int gcd(int a, int b) {
        while (b != 0) {
            int temp = b;
            b = a % b;
            a = temp;
        }
        return a;
    }

public:
    PointingIndexSystem(JsonReaderSystem* reader, MultiDimensionalPointingSystem* pointing) 
        : jsonReader(reader), pointingSystem(pointing) {
        initializeRegistry();
        buildPointingIndex();
    }
    
    void buildPointingIndex() {
        // Load all entries from JSON reader
        loadAllEntries();
        
        // Build text and category indices
        buildTextIndex();
        
        // Pre-compute similarity matrix for frequently accessed pairs
        precomputeSimilarities();
        
        std::cout << "Built pointing index with " << allEntries.size() << " entries" << std::endl;
        std::cout << "Registry contains " << registryKeys.size() << " dynamic properties" << std::endl;
        std::cout << "Text index covers " << textIndex.size() << " property values" << std::endl;
    }
    
    void loadAllEntries() {
        // Load from all JSON data sources
        const auto& guitarData = jsonReader->getGuitarData();
        const auto& groupData = jsonReader->getGroupData();
        const auto& moodsData = jsonReader->getMoodsData();
        const auto& structureData = jsonReader->getStructureData();
        const auto& synthData = jsonReader->getSynthData();
        
        // Process guitar entries
        if (guitarData.contains("groups")) {
            for (const auto& [key, entry] : guitarData["groups"].items()) {
                if (entry.contains("id")) {
                    IndexedEntry indexed;
                    indexed.id = entry["id"];
                    indexed.entry = entry;
                    indexed.parsedId = jsonReader->parseId(indexed.id);
                    
                    // Extract dynamic vector
                    extractPropertyVector(entry, indexed.dynamicVector);
                    
                    // Convert to ordered vector
                    indexed.embedding.reserve(registryKeys.size());
                    float magnitude = 0.0f;
                    for (const auto& key : registryKeys) {
                        float value = indexed.dynamicVector[key];
                        indexed.embedding.push_back(value);
                        magnitude += value * value;
                    }
                    indexed.vectorMagnitude = std::sqrt(magnitude);
                    
                    idToIndex[indexed.id] = allEntries.size();
                    allEntries.push_back(indexed);
                }
            }
        }
        
        // Process group entries
        if (groupData.contains("groups")) {
            for (const auto& [key, entry] : groupData["groups"].items()) {
                if (entry.contains("id")) {
                    IndexedEntry indexed;
                    indexed.id = entry["id"];
                    indexed.entry = entry;
                    indexed.parsedId = jsonReader->parseId(indexed.id);
                    
                    extractPropertyVector(entry, indexed.dynamicVector);
                    
                    indexed.embedding.reserve(registryKeys.size());
                    float magnitude = 0.0f;
                    for (const auto& key : registryKeys) {
                        float value = indexed.dynamicVector[key];
                        indexed.embedding.push_back(value);
                        magnitude += value * value;
                    }
                    indexed.vectorMagnitude = std::sqrt(magnitude);
                    
                    idToIndex[indexed.id] = allEntries.size();
                    allEntries.push_back(indexed);
                }
            }
        }
        
        // Continue for other data sources...
        // (Similar processing for moods, structure, and synthesizer data)
    }
    
    void precomputeSimilarities() {
        size_t n = allEntries.size();
        precomputedSimilarity.resize(n, std::vector<float>(n, 0.0f));
        
        for (size_t i = 0; i < n; i++) {
            precomputedSimilarity[i][i] = 1.0f;
            for (size_t j = i + 1; j < n; j++) {
                float similarity = calculateVectorSimilarity(
                    allEntries[i].embedding, allEntries[j].embedding);
                precomputedSimilarity[i][j] = similarity;
                precomputedSimilarity[j][i] = similarity;
            }
        }
    }
    
    std::vector<SearchResult> search(const std::string& queryId, 
                                   const std::unordered_map<std::string, std::string>& filters = {},
                                   int maxResults = 10) {
        std::vector<SearchResult> results;
        
        // Find query entry
        auto queryIt = idToIndex.find(queryId);
        if (queryIt == idToIndex.end()) {
            return results; // Empty if query not found
        }
        
        size_t queryIndex = queryIt->second;
        const IndexedEntry& queryEntry = allEntries[queryIndex];
        
        // Score all other entries
        for (size_t i = 0; i < allEntries.size(); i++) {
            if (i == queryIndex) continue;
            
            SearchResult result;
            result.entry = allEntries[i];
            
            // Apply pre-filters based on ID proximity
            std::vector<std::string> idExplanations;
            result.idProximityScore = calculateIdProximity(
                queryEntry.parsedId, allEntries[i].parsedId, idExplanations);
            
            // Skip if ID proximity is too low (efficiency filter)
            if (result.idProximityScore < 0.1f) continue;
            
            // Vector similarity score
            if (i < precomputedSimilarity.size() && queryIndex < precomputedSimilarity[i].size()) {
                result.vectorScore = precomputedSimilarity[queryIndex][i];
            } else {
                result.vectorScore = calculateVectorSimilarity(
                    queryEntry.embedding, allEntries[i].embedding);
            }
            
            // Semantic scoring based on shared properties
            result.semanticScore = calculateSemanticMatch(queryEntry.entry, allEntries[i].entry);
            
            // Weighted final score
            result.finalScore = (result.vectorScore * 0.4f +
                               result.semanticScore * 0.3f +
                               result.idProximityScore * 0.3f);
            
            // Add explanations
            result.matchReasons.insert(result.matchReasons.end(), 
                                     idExplanations.begin(), idExplanations.end());
            
            if (result.vectorScore > 0.7f) {
                result.matchReasons.push_back("High vector similarity: " + 
                                            std::to_string(result.vectorScore));
            }
            
            // Mark creative matches
            if (result.idProximityScore > 0.3f && result.vectorScore > 0.6f) {
                result.isCreativeMatch = true;
                result.matchReasons.push_back("Creative match: ID proximity + vector similarity");
            }
            
            // Apply filters
            bool passesFilters = true;
            for (const auto& [filterKey, filterValue] : filters) {
                if (filterKey == "soundGeneration") {
                    if (allEntries[i].entry.value("soundGeneration", "") != filterValue) {
                        passesFilters = false;
                        break;
                    }
                } else if (filterKey == "frequencyRange") {
                    if (allEntries[i].entry.value("frequencyRange", "") != filterValue) {
                        passesFilters = false;
                        break;
                    }
                }
                // Add more filter types as needed
            }
            
            if (passesFilters && result.finalScore > 0.2f) {
                results.push_back(result);
            }
        }
        
        // Sort by final score
        std::sort(results.begin(), results.end(),
                 [](const SearchResult& a, const SearchResult& b) {
                     return a.finalScore > b.finalScore;
                 });
        
        // Limit results
        if (results.size() > maxResults) {
            results.resize(maxResults);
        }
        
        return results;
    }
    
    void addNewProperty(const std::string& propertyName, float defaultValue = 0.5f) {
        // Add to registry if not exists
        if (std::find(registryKeys.begin(), registryKeys.end(), propertyName) == registryKeys.end()) {
            registryKeys.push_back(propertyName);
            globalRegistry[propertyName] = defaultValue;
            
            // Re-index all entries with new property
            reindexWithNewProperty(propertyName, defaultValue);
            
            std::cout << "Added new property: " << propertyName << std::endl;
            std::cout << "Re-indexed " << allEntries.size() << " entries" << std::endl;
        }
    }
    
private:
    float calculateSemanticMatch(const json& a, const json& b) {
        float score = 0.0f;
        
        // Check shared FX categories
        if (a.contains("fxCategories") && b.contains("fxCategories")) {
            auto fxA = a["fxCategories"];
            auto fxB = b["fxCategories"];
            if (fxA.is_array() && fxB.is_array()) {
                std::unordered_set<std::string> setA, setB;
                for (const auto& fx : fxA) {
                    if (fx.is_string()) setA.insert(fx);
                }
                for (const auto& fx : fxB) {
                    if (fx.is_string()) setB.insert(fx);
                }
                
                size_t intersection = 0;
                for (const auto& fx : setA) {
                    if (setB.count(fx)) intersection++;
                }
                
                if (!setA.empty() && !setB.empty()) {
                    score += 0.4f * float(intersection) / float(std::max(setA.size(), setB.size()));
                }
            }
        }
        
        // Check same sound generation
        if (a.value("soundGeneration", "") == b.value("soundGeneration", "") &&
            !a.value("soundGeneration", "").empty()) {
            score += 0.3f;
        }
        
        // Check compatible frequency ranges
        std::string freqA = a.value("frequencyRange", "");
        std::string freqB = b.value("frequencyRange", "");
        if (!freqA.empty() && !freqB.empty()) {
            if (freqA == freqB) score += 0.1f;
            else if (freqA == "full-spectrum" || freqB == "full-spectrum") score += 0.2f;
        }
        
        // Check theory tuning compatibility
        if (a.value("theoryTuning", "") == b.value("theoryTuning", "") &&
            !a.value("theoryTuning", "").empty()) {
            score += 0.2f;
        }
        
        return std::min(1.0f, score);
    }
    
    void reindexWithNewProperty(const std::string& propertyName, float defaultValue) {
        // Re-extract vectors for all entries
        for (auto& indexed : allEntries) {
            // Re-extract full property vector
            indexed.dynamicVector.clear();
            extractPropertyVector(indexed.entry, indexed.dynamicVector);
            
            // Rebuild embedding vector
            indexed.embedding.clear();
            indexed.embedding.reserve(registryKeys.size());
            float magnitude = 0.0f;
            for (const auto& key : registryKeys) {
                float value = indexed.dynamicVector.count(key) ? indexed.dynamicVector[key] : defaultValue;
                indexed.embedding.push_back(value);
                magnitude += value * value;
            }
            indexed.vectorMagnitude = std::sqrt(magnitude);
        }
        
        // Re-compute similarity matrix
        precomputeSimilarities();
    }
    
public:
    // Diagnostic and analysis functions
    void printIndexStatistics() {
        std::cout << "\n=== POINTING INDEX SYSTEM STATISTICS ===" << std::endl;
        std::cout << "Total indexed entries: " << allEntries.size() << std::endl;
        std::cout << "Registry properties: " << registryKeys.size() << std::endl;
        std::cout << "Text index categories: " << textIndex.size() << std::endl;
        std::cout << "Category index: " << categoryIndex.size() << std::endl;
        
        // Property distribution analysis
        std::cout << "\nProperty value distributions:" << std::endl;
        for (const auto& key : registryKeys) {
            std::vector<float> values;
            for (const auto& entry : allEntries) {
                if (entry.dynamicVector.count(key)) {
                    values.push_back(entry.dynamicVector.at(key));
                }
            }
            
            if (!values.empty()) {
                std::sort(values.begin(), values.end());
                float median = values[values.size() / 2];
                float mean = std::accumulate(values.begin(), values.end(), 0.0f) / values.size();
                std::cout << "  " << key << ": mean=" << mean << ", median=" << median << std::endl;
            }
        }
        
        std::cout << "===========================================" << std::endl;
    }
    
    json exportIndexedData() {
        json export_data;
        export_data["metadata"]["version"] = "4Z-Index-1.0";
        export_data["metadata"]["total_entries"] = allEntries.size();
        export_data["metadata"]["registry_size"] = registryKeys.size();
        
        export_data["registry_keys"] = registryKeys;
        
        json entries_array = json::array();
        for (const auto& indexed : allEntries) {
            json entry_data;
            entry_data["id"] = indexed.id;
            entry_data["embedding"] = indexed.embedding;
            entry_data["dynamic_vector"] = indexed.dynamicVector;
            entry_data["vector_magnitude"] = indexed.vectorMagnitude;
            entries_array.push_back(entry_data);
        }
        export_data["indexed_entries"] = entries_array;
        
        return export_data;
    }
};