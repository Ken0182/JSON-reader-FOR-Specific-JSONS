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

using namespace std;
using json = nlohmann::json;

// Forward declarations
class EmbeddingEngine;
class PointingIndex;
class SearchRanker;

// Configuration entry with full metadata
struct ConfigEntry {
    string path;                    // e.g., "Classical_Nylon_Soft.adsr.attack"
    string instrumentName;          // e.g., "Classical_Nylon_Soft"
    string category;               // e.g., "guitar", "group"
    string fieldType;              // e.g., "adsr", "oscillator", "effects"
    json value;                    // The actual value
    vector<string> tags;           // Extracted semantic tags
    string explanation;            // Why this entry is relevant
    vector<float> embedding;       // Vector representation
    float boostScore = 1.0f;       // Learning-based boost/demote
    map<string, string> metadata;  // Additional context
};

// Search result with explainability
struct SearchResult {
    ConfigEntry entry;
    float textScore = 0.0f;
    float vectorScore = 0.0f;
    float finalScore = 0.0f;
    vector<string> matchReasons;   // Why this matched
    vector<string> relatedPaths;   // Related suggestions
    string explanation;            // Human-readable explanation
};

// User interaction context
struct UserContext {
    vector<string> selectedPaths;
    vector<string> excludedPaths;
    vector<string> searchHistory;
    map<string, float> preferences; // Learned preferences
    string currentQuery;
    string sessionId;
};

// Simple embedding engine (placeholder for real implementation)
class EmbeddingEngine {
private:
    map<string, vector<float>> wordEmbeddings;
    map<string, vector<float>> cachedEmbeddings;
    
public:
    EmbeddingEngine() {
        loadPretrainedEmbeddings();
    }
    
    void loadPretrainedEmbeddings() {
        // Simulate loading pretrained embeddings
        // In real implementation, load FastText/BERT embeddings
        cout << "Loading pretrained embeddings..." << endl;
        
        // Example semantic embeddings for music terms
        wordEmbeddings["warm"] = {0.8f, 0.6f, 0.3f, 0.9f, 0.2f};
        wordEmbeddings["bright"] = {0.2f, 0.9f, 0.8f, 0.4f, 0.7f};
        wordEmbeddings["aggressive"] = {0.9f, 0.3f, 0.8f, 0.1f, 0.6f};
        wordEmbeddings["calm"] = {0.3f, 0.2f, 0.1f, 0.8f, 0.9f};
        wordEmbeddings["guitar"] = {0.7f, 0.5f, 0.4f, 0.6f, 0.3f};
        wordEmbeddings["bass"] = {0.9f, 0.2f, 0.3f, 0.7f, 0.4f};
        wordEmbeddings["reverb"] = {0.4f, 0.7f, 0.6f, 0.5f, 0.8f};
        wordEmbeddings["attack"] = {0.8f, 0.9f, 0.2f, 0.3f, 0.4f};
        wordEmbeddings["sustain"] = {0.3f, 0.4f, 0.9f, 0.8f, 0.5f};
        
        cout << "Loaded " << wordEmbeddings.size() << " word embeddings." << endl;
    }
    
    vector<float> getEmbedding(const string& text) {
        string key = toLowerCase(text);
        
        if (cachedEmbeddings.find(key) != cachedEmbeddings.end()) {
            return cachedEmbeddings[key];
        }
        
        vector<float> embedding = computeTextEmbedding(text);
        cachedEmbeddings[key] = embedding;
        return embedding;
    }
    
private:
    string toLowerCase(const string& str) {
        string result = str;
        transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }
    
    vector<float> computeTextEmbedding(const string& text) {
        // Simple averaging of word embeddings
        vector<float> result(5, 0.0f); // 5-dimensional embeddings
        int wordCount = 0;
        
        stringstream ss(text);
        string word;
        while (ss >> word) {
            word = toLowerCase(word);
            word = regex_replace(word, regex("[^a-zA-Z]"), "");
            
            if (wordEmbeddings.find(word) != wordEmbeddings.end()) {
                auto& wordEmb = wordEmbeddings[word];
                for (size_t i = 0; i < result.size() && i < wordEmb.size(); ++i) {
                    result[i] += wordEmb[i];
                }
                wordCount++;
            }
        }
        
        if (wordCount > 0) {
            for (float& val : result) {
                val /= wordCount;
            }
        }
        
        return result;
    }
    
public:
    float computeSimilarity(const vector<float>& a, const vector<float>& b) {
        if (a.size() != b.size()) return 0.0f;
        
        float dotProduct = 0.0f;
        float normA = 0.0f;
        float normB = 0.0f;
        
        for (size_t i = 0; i < a.size(); ++i) {
            dotProduct += a[i] * b[i];
            normA += a[i] * a[i];
            normB += b[i] * b[i];
        }
        
        if (normA == 0.0f || normB == 0.0f) return 0.0f;
        
        return dotProduct / (sqrt(normA) * sqrt(normB));
    }
};

// Extended SKD with explanations and embeddings
class SemanticKeywordDatabase {
private:
    json skdData;
    EmbeddingEngine* embeddingEngine;
    
public:
    SemanticKeywordDatabase(EmbeddingEngine* engine) : embeddingEngine(engine) {
        loadExtendedSKD();
    }
    
    void loadExtendedSKD() {
        skdData = json::object();
        
        // Extended SKD with explanations and context
        skdData["warm"] = {
            {"category", "timbral"},
            {"aliases", json::array({"soft", "mellow", "cozy"})},
            {"score", 0.9},
            {"explanation", "Produces soft, comfortable tones with rounded harmonics, ideal for intimate musical passages"},
            {"context", json::array({"acoustic", "classical", "jazz"})},
            {"opposites", json::array({"bright", "harsh", "cold"})}
        };
        
        skdData["bright"] = {
            {"category", "timbral"},
            {"aliases", json::array({"shiny", "clear", "crisp"})},
            {"score", 0.85},
            {"explanation", "Creates clear, cutting tones with enhanced high frequencies, perfect for lead instruments"},
            {"context", json::array({"electric", "lead", "pop"})},
            {"opposites", json::array({"warm", "dull", "muffled"})}
        };
        
        skdData["aggressive"] = {
            {"category", "emotional"},
            {"aliases", json::array({"intense", "fierce", "driving"})},
            {"score", 0.9},
            {"explanation", "Delivers powerful, assertive sounds with strong attack and presence for energetic sections"},
            {"context", json::array({"rock", "metal", "electronic"})},
            {"opposites", json::array({"calm", "gentle", "subtle"})}
        };
        
        skdData["calm"] = {
            {"category", "emotional"},
            {"aliases", json::array({"peaceful", "relaxed", "serene"})},
            {"score", 0.8},
            {"explanation", "Produces soothing, tranquil sounds with gentle dynamics for reflective moments"},
            {"context", json::array({"ambient", "classical", "meditation"})},
            {"opposites", json::array({"aggressive", "energetic", "chaotic"})}
        };
        
        skdData["attack"] = {
            {"category", "parameter"},
            {"aliases", json::array({"onset", "start", "initial"})},
            {"score", 0.95},
            {"explanation", "Controls how quickly a sound reaches full volume - fast for percussive, slow for pads"},
            {"context", json::array({"envelope", "dynamics", "timing"})},
            {"relatedParams", json::array({"decay", "sustain", "release"})}
        };
        
        skdData["reverb"] = {
            {"category", "effect"},
            {"aliases", json::array({"echo", "space", "ambience"})},
            {"score", 0.85},
            {"explanation", "Adds spatial depth and ambience, simulating acoustic spaces from rooms to halls"},
            {"context", json::array({"space", "depth", "atmosphere"})},
            {"relatedEffects", json::array({"delay", "chorus", "hall"})}
        };
        
        // Compute and cache embeddings for all entries
        for (auto& [key, entry] : skdData.items()) {
            string combinedText = key;
            if (entry.contains("explanation")) {
                combinedText += " " + entry["explanation"].get<string>();
            }
            if (entry.contains("aliases")) {
                for (const auto& alias : entry["aliases"]) {
                    combinedText += " " + alias.get<string>();
                }
            }
            
            entry["embedding"] = embeddingEngine->getEmbedding(combinedText);
        }
        
        cout << "Loaded extended SKD with " << skdData.size() << " entries and embeddings." << endl;
    }
    
    json getEntry(const string& key) {
        string lowerKey = toLowerCase(key);
        if (skdData.contains(lowerKey)) {
            return skdData[lowerKey];
        }
        return json::object();
    }
    
    vector<string> findRelatedTerms(const string& key, float threshold = 0.7f) {
        vector<string> related;
        auto keyEntry = getEntry(key);
        
        if (keyEntry.empty() || !keyEntry.contains("embedding")) {
            return related;
        }
        
        vector<float> keyEmbedding = keyEntry["embedding"];
        
        for (const auto& [term, entry] : skdData.items()) {
            if (term == toLowerCase(key)) continue;
            
            if (entry.contains("embedding")) {
                vector<float> termEmbedding = entry["embedding"];
                float similarity = embeddingEngine->computeSimilarity(keyEmbedding, termEmbedding);
                
                if (similarity >= threshold) {
                    related.push_back(term);
                }
            }
        }
        
        return related;
    }
    
private:
    string toLowerCase(const string& str) {
        string result = str;
        transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }
};

// Main Pointing Index System
class PointingIndex {
private:
    vector<ConfigEntry> allEntries;
    map<string, vector<size_t>> textIndex;     // word -> entry indices
    map<string, vector<size_t>> pathIndex;    // path -> entry indices
    map<string, vector<size_t>> categoryIndex; // category -> entry indices
    EmbeddingEngine embeddingEngine;
    SemanticKeywordDatabase skd;
    json cleanConfig;
    json referenceData;
    
public:
    PointingIndex() : skd(&embeddingEngine) {
        loadAllData();
        buildPointingIndex();
    }
    
private:
    void loadAllData() {
        cout << "Loading all configuration data..." << endl;
        
        // Load clean config (actual renderable data)
        ifstream configFile("clean_config.json");
        if (configFile) {
            configFile >> cleanConfig;
            cout << "Loaded clean config with " << cleanConfig.size() << " instruments/groups." << endl;
        }
        
        // Load reference data for vectorization/scoring
        ifstream moodsFile("moods.json");
        if (moodsFile) {
            json moods;
            moodsFile >> moods;
            referenceData["moods"] = moods;
        }
        
        ifstream synthFile("Synthesizer.json");
        if (synthFile) {
            json synth;
            synthFile >> synth;
            referenceData["synthesizer"] = synth;
        }
        
        cout << "Loaded reference data for vectorization." << endl;
    }
    
    void buildPointingIndex() {
        cout << "Building pointing index..." << endl;
        auto startTime = chrono::high_resolution_clock::now();
        
        allEntries.clear();
        textIndex.clear();
        pathIndex.clear();
        categoryIndex.clear();
        
        // Index clean config entries (actual renderable data)
        indexConfigEntries();
        
        // Build text indexes
        buildTextIndexes();
        
        auto endTime = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);
        
        cout << "Built pointing index with " << allEntries.size() << " entries in " 
             << duration.count() << "ms." << endl;
    }
    
    void indexConfigEntries() {
        for (const auto& [instrumentName, instrumentData] : cleanConfig.items()) {
            if (!instrumentData.is_object()) continue;
            
            string category = determineCategory(instrumentName, instrumentData);
            
            // Index the instrument itself
            ConfigEntry instrumentEntry;
            instrumentEntry.path = instrumentName;
            instrumentEntry.instrumentName = instrumentName;
            instrumentEntry.category = category;
            instrumentEntry.fieldType = "instrument";
            instrumentEntry.value = instrumentData;
            instrumentEntry.tags = extractTags(instrumentData);
            instrumentEntry.explanation = generateExplanation(instrumentName, instrumentData);
            instrumentEntry.embedding = embeddingEngine.getEmbedding(
                instrumentName + " " + instrumentEntry.explanation
            );
            
            allEntries.push_back(instrumentEntry);
            
            // Recursively index all fields
            indexFieldsRecursively(instrumentName, category, "", instrumentData);
        }
    }
    
    void indexFieldsRecursively(const string& instrumentName, const string& category, 
                               const string& currentPath, const json& data) {
        if (data.is_object()) {
            for (const auto& [key, value] : data.items()) {
                string newPath = currentPath.empty() ? key : currentPath + "." + key;
                string fullPath = instrumentName + "." + newPath;
                
                ConfigEntry entry;
                entry.path = fullPath;
                entry.instrumentName = instrumentName;
                entry.category = category;
                entry.fieldType = determineFieldType(key, value);
                entry.value = value;
                entry.tags = extractTags(value);
                entry.explanation = generateFieldExplanation(key, value, instrumentName);
                
                // Add context from parent
                string contextText = key + " " + entry.explanation;
                if (data.contains("timbral")) {
                    contextText += " " + data["timbral"].get<string>();
                }
                
                entry.embedding = embeddingEngine.getEmbedding(contextText);
                
                allEntries.push_back(entry);
                
                // Recurse into nested objects
                if (value.is_object()) {
                    indexFieldsRecursively(instrumentName, category, newPath, value);
                }
            }
        }
    }
    
    string determineCategory(const string& name, const json& data) {
        if (data.contains("guitarParams")) return "guitar";
        if (data.contains("synthesisType")) return "group";
        if (name.find("Bass") != string::npos) return "bass";
        if (name.find("Lead") != string::npos) return "lead";
        if (name.find("Pad") != string::npos) return "pad";
        return "instrument";
    }
    
    string determineFieldType(const string& key, const json& value) {
        if (key == "adsr" || key == "envelope") return "envelope";
        if (key == "oscillator") return "oscillator";
        if (key == "filter") return "filter";
        if (key == "effects" || key == "fx") return "effects";
        if (key == "soundCharacteristics") return "characteristics";
        if (key == "guitarParams") return "guitar_specific";
        if (value.is_array()) return "array";
        if (value.is_number()) return "parameter";
        if (value.is_string()) return "property";
        return "complex";
    }
    
    vector<string> extractTags(const json& data) {
        vector<string> tags;
        
        if (data.is_object()) {
            if (data.contains("timbral") && data["timbral"].is_string()) {
                tags.push_back(data["timbral"].get<string>());
            }
            if (data.contains("material") && data["material"].is_string()) {
                tags.push_back(data["material"].get<string>());
            }
            if (data.contains("dynamic") && data["dynamic"].is_string()) {
                tags.push_back(data["dynamic"].get<string>());
            }
            if (data.contains("emotional") && data["emotional"].is_array()) {
                for (const auto& emotion : data["emotional"]) {
                    if (emotion.is_object() && emotion.contains("tag")) {
                        tags.push_back(emotion["tag"].get<string>());
                    }
                }
            }
        } else if (data.is_string()) {
            tags.push_back(data.get<string>());
        }
        
        return tags;
    }
    
    string generateExplanation(const string& name, const json& data) {
        string explanation = "Instrument configuration for " + name;
        
        if (data.contains("soundCharacteristics") && data["soundCharacteristics"].is_object()) {
            const auto& chars = data["soundCharacteristics"];
            if (chars.contains("timbral")) {
                explanation += " with " + chars["timbral"].get<string>() + " timbral character";
            }
            if (chars.contains("dynamic")) {
                explanation += " and " + chars["dynamic"].get<string>() + " dynamics";
            }
        }
        
        if (data.contains("synthesisType")) {
            explanation += " using " + data["synthesisType"].get<string>() + " synthesis";
        }
        
        return explanation;
    }
    
    string generateFieldExplanation(const string& key, const json& value, const string& instrument) {
        auto skdEntry = skd.getEntry(key);
        if (!skdEntry.empty() && skdEntry.contains("explanation")) {
            return skdEntry["explanation"].get<string>();
        }
        
        // Generate context-aware explanations
        if (key == "attack") {
            return "Controls how quickly the sound reaches full volume when a note is triggered";
        } else if (key == "decay") {
            return "Sets how quickly the sound drops from peak to sustain level";
        } else if (key == "sustain") {
            return "Determines the level at which the sound is held while a note is pressed";
        } else if (key == "release") {
            return "Controls how quickly the sound fades when a note is released";
        } else if (key == "cutoff") {
            return "Sets the frequency above which the filter attenuates the signal";
        } else if (key == "resonance") {
            return "Adds emphasis at the filter cutoff frequency for more character";
        } else if (key == "reverb") {
            return "Adds spatial depth and ambience to simulate acoustic spaces";
        } else if (key == "delay") {
            return "Creates echo effects by repeating the signal with time offset";
        }
        
        return "Parameter '" + key + "' for " + instrument;
    }
    
    void buildTextIndexes() {
        for (size_t i = 0; i < allEntries.size(); ++i) {
            const auto& entry = allEntries[i];
            
            // Index path components
            pathIndex[entry.path].push_back(i);
            pathIndex[entry.instrumentName].push_back(i);
            pathIndex[entry.fieldType].push_back(i);
            
            // Index category
            categoryIndex[entry.category].push_back(i);
            
            // Index words from explanation and tags
            indexWords(entry.explanation, i);
            for (const string& tag : entry.tags) {
                indexWords(tag, i);
            }
            
            // Index JSON value text
            if (entry.value.is_string()) {
                indexWords(entry.value.get<string>(), i);
            }
        }
    }
    
    void indexWords(const string& text, size_t entryIndex) {
        stringstream ss(text);
        string word;
        while (ss >> word) {
            // Clean and normalize word
            word = regex_replace(word, regex("[^a-zA-Z0-9]"), "");
            transform(word.begin(), word.end(), word.begin(), ::tolower);
            
            if (!word.empty()) {
                textIndex[word].push_back(entryIndex);
            }
        }
    }

public:
    // Search functionality
    vector<SearchResult> search(const string& query, const UserContext& context, int maxResults = 10) {
        cout << "\n=== SEARCH: \"" << query << "\" ===" << endl;
        
        vector<SearchResult> results;
        vector<float> queryEmbedding = embeddingEngine.getEmbedding(query);
        
        for (size_t i = 0; i < allEntries.size(); ++i) {
            const auto& entry = allEntries[i];
            
            // Skip excluded paths
            if (find(context.excludedPaths.begin(), context.excludedPaths.end(), 
                    entry.path) != context.excludedPaths.end()) {
                continue;
            }
            
            SearchResult result;
            result.entry = entry;
            
            // Text-based scoring
            result.textScore = computeTextScore(query, entry);
            
            // Vector-based scoring
            result.vectorScore = embeddingEngine.computeSimilarity(queryEmbedding, entry.embedding);
            
            // Apply user preferences and learning
            float userBoost = 1.0f;
            if (context.preferences.find(entry.category) != context.preferences.end()) {
                userBoost = context.preferences.at(entry.category);
            }
            
            // Weighted final score
            result.finalScore = (0.4f * result.textScore + 0.6f * result.vectorScore) 
                              * entry.boostScore * userBoost;
            
            // Generate match reasons
            result.matchReasons = generateMatchReasons(query, entry, result);
            
            // Find related paths
            result.relatedPaths = findRelatedPaths(entry);
            
            // Generate explanation
            result.explanation = generateSearchExplanation(entry, result);
            
            if (result.finalScore > 0.1f) { // Minimum threshold
                results.push_back(result);
            }
        }
        
        // Sort by final score
        sort(results.begin(), results.end(), 
             [](const SearchResult& a, const SearchResult& b) {
                 return a.finalScore > b.finalScore;
             });
        
        // Limit results
        if (results.size() > maxResults) {
            results.resize(maxResults);
        }
        
        // Log search results
        logSearchResults(query, results);
        
        return results;
    }
    
private:
    float computeTextScore(const string& query, const ConfigEntry& entry) {
        float score = 0.0f;
        string lowerQuery = toLowerCase(query);
        
        // Exact path match
        if (toLowerCase(entry.path).find(lowerQuery) != string::npos) {
            score += 1.0f;
        }
        
        // Instrument name match
        if (toLowerCase(entry.instrumentName).find(lowerQuery) != string::npos) {
            score += 0.8f;
        }
        
        // Tag exact matches
        for (const string& tag : entry.tags) {
            if (toLowerCase(tag) == lowerQuery) {
                score += 0.9f;
            } else if (toLowerCase(tag).find(lowerQuery) != string::npos) {
                score += 0.6f;
            }
        }
        
        // Explanation text match
        if (toLowerCase(entry.explanation).find(lowerQuery) != string::npos) {
            score += 0.5f;
        }
        
        // SKD alias matching
        auto skdEntry = skd.getEntry(lowerQuery);
        if (!skdEntry.empty() && skdEntry.contains("aliases")) {
            for (const auto& alias : skdEntry["aliases"]) {
                string aliasStr = alias.get<string>();
                for (const string& tag : entry.tags) {
                    if (toLowerCase(tag) == toLowerCase(aliasStr)) {
                        score += 0.7f;
                    }
                }
            }
        }
        
        return min(score, 2.0f); // Cap score
    }
    
    vector<string> generateMatchReasons(const string& query, const ConfigEntry& entry, 
                                      const SearchResult& result) {
        vector<string> reasons;
        string lowerQuery = toLowerCase(query);
        
        if (result.textScore > 0.8f) {
            reasons.push_back("Direct text match in " + entry.fieldType);
        }
        
        if (result.vectorScore > 0.7f) {
            reasons.push_back("High semantic similarity");
        }
        
        for (const string& tag : entry.tags) {
            if (toLowerCase(tag).find(lowerQuery) != string::npos) {
                reasons.push_back("Tag match: '" + tag + "'");
            }
        }
        
        if (toLowerCase(entry.category).find(lowerQuery) != string::npos) {
            reasons.push_back("Category match: " + entry.category);
        }
        
        return reasons;
    }
    
    vector<string> findRelatedPaths(const ConfigEntry& entry) {
        vector<string> related;
        
        // Find other entries from same instrument
        for (const auto& other : allEntries) {
            if (other.instrumentName == entry.instrumentName && 
                other.path != entry.path && related.size() < 3) {
                related.push_back(other.path);
            }
        }
        
        // Find entries with similar tags
        for (const string& tag : entry.tags) {
            auto relatedTerms = skd.findRelatedTerms(tag, 0.8f);
            for (const string& term : relatedTerms) {
                if (related.size() >= 5) break;
                related.push_back("Related: " + term);
            }
        }
        
        return related;
    }
    
    string generateSearchExplanation(const ConfigEntry& entry, const SearchResult& result) {
        stringstream explanation;
        
        explanation << "Score: " << fixed << setprecision(2) << result.finalScore;
        explanation << " (Text: " << result.textScore << ", Vector: " << result.vectorScore << ")";
        explanation << " - " << entry.explanation;
        
        if (!result.matchReasons.empty()) {
            explanation << " | Matches: ";
            for (size_t i = 0; i < result.matchReasons.size(); ++i) {
                if (i > 0) explanation << ", ";
                explanation << result.matchReasons[i];
            }
        }
        
        return explanation.str();
    }
    
    void logSearchResults(const string& query, const vector<SearchResult>& results) {
        cout << "Found " << results.size() << " results for query: '" << query << "'" << endl;
        
        for (size_t i = 0; i < min((size_t)5, results.size()); ++i) {
            const auto& result = results[i];
            cout << (i + 1) << ". " << result.entry.path 
                 << " (Score: " << fixed << setprecision(2) << result.finalScore << ")" << endl;
            cout << "   " << result.explanation << endl;
        }
    }
    
    string toLowerCase(const string& str) {
        string result = str;
        transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

public:
    // Interactive operations
    vector<SearchResult> moreLikeThis(const string& path, const UserContext& context) {
        cout << "\n=== MORE LIKE: " << path << " ===" << endl;
        
        // Find the reference entry
        const ConfigEntry* refEntry = nullptr;
        for (const auto& entry : allEntries) {
            if (entry.path == path) {
                refEntry = &entry;
                break;
            }
        }
        
        if (!refEntry) {
            cout << "Reference entry not found: " << path << endl;
            return {};
        }
        
        // Build similarity query from reference entry
        string similarityQuery = refEntry->explanation;
        for (const string& tag : refEntry->tags) {
            similarityQuery += " " + tag;
        }
        
        return search(similarityQuery, context);
    }
    
    void recordUserChoice(const string& path, bool positive, UserContext& context) {
        cout << "\n=== LEARNING: " << path << " (" << (positive ? "BOOST" : "DEMOTE") << ") ===" << endl;
        
        // Update entry boost score
        for (auto& entry : allEntries) {
            if (entry.path == path) {
                if (positive) {
                    entry.boostScore = min(entry.boostScore + 0.1f, 2.0f);
                    cout << "Boosted " << path << " to " << entry.boostScore << endl;
                } else {
                    entry.boostScore = max(entry.boostScore - 0.1f, 0.1f);
                    cout << "Demoted " << path << " to " << entry.boostScore << endl;
                }
                break;
            }
        }
        
        // Update user preferences for category
        for (const auto& entry : allEntries) {
            if (entry.path == path) {
                if (context.preferences.find(entry.category) == context.preferences.end()) {
                    context.preferences[entry.category] = 1.0f;
                }
                
                if (positive) {
                    context.preferences[entry.category] = min(context.preferences[entry.category] + 0.05f, 1.5f);
                } else {
                    context.preferences[entry.category] = max(context.preferences[entry.category] - 0.05f, 0.5f);
                }
                
                cout << "Updated " << entry.category << " preference to " 
                     << context.preferences[entry.category] << endl;
                break;
            }
        }
    }
    
    void printIndexStatistics() {
        cout << "\n=== POINTING INDEX STATISTICS ===" << endl;
        cout << "Total entries: " << allEntries.size() << endl;
        cout << "Text index terms: " << textIndex.size() << endl;
        cout << "Path index entries: " << pathIndex.size() << endl;
        cout << "Categories: " << categoryIndex.size() << endl;
        
        map<string, int> categoryStats;
        for (const auto& entry : allEntries) {
            categoryStats[entry.category]++;
        }
        
        cout << "Entries by category:" << endl;
        for (const auto& [category, count] : categoryStats) {
            cout << "  " << category << ": " << count << endl;
        }
        
        cout << "SKD terms loaded: ";
        auto relatedToWarm = skd.findRelatedTerms("warm");
        cout << relatedToWarm.size() << " terms related to 'warm'" << endl;
        
        cout << "=================================" << endl;
    }
    
    // Get clean config for rendering (only actual usable data)
    json getCleanConfigForSynthesis() {
        return cleanConfig;
    }
};

// Interactive session manager
class PointingSession {
private:
    PointingIndex index;
    UserContext context;
    
public:
    PointingSession() {
        context.sessionId = generateSessionId();
        cout << "Started pointing session: " << context.sessionId << endl;
    }
    
    void runInteractiveSession() {
        cout << "\n=== POINTING INDEX INTERACTIVE SESSION ===" << endl;
        cout << "Commands: search <query>, like <path>, exclude <path>, boost <path>, demote <path>, stats, config, quit" << endl;
        
        string input;
        while (true) {
            cout << "\n> ";
            getline(cin, input);
            
            if (input.empty()) continue;
            
            vector<string> parts = splitCommand(input);
            if (parts.empty()) continue;
            
            string command = parts[0];
            
            if (command == "quit" || command == "exit") {
                break;
            } else if (command == "search" && parts.size() > 1) {
                string query = input.substr(7); // Skip "search "
                auto results = index.search(query, context);
                displaySearchResults(results);
            } else if (command == "like" && parts.size() > 1) {
                string path = parts[1];
                auto results = index.moreLikeThis(path, context);
                displaySearchResults(results);
            } else if (command == "exclude" && parts.size() > 1) {
                string path = parts[1];
                context.excludedPaths.push_back(path);
                cout << "Excluded: " << path << endl;
            } else if (command == "boost" && parts.size() > 1) {
                string path = parts[1];
                index.recordUserChoice(path, true, context);
            } else if (command == "demote" && parts.size() > 1) {
                string path = parts[1];
                index.recordUserChoice(path, false, context);
            } else if (command == "stats") {
                index.printIndexStatistics();
                printUserStats();
            } else if (command == "config") {
                cout << "Clean config available for synthesis with " 
                     << index.getCleanConfigForSynthesis().size() << " instruments/groups." << endl;
            } else {
                cout << "Unknown command. Try: search, like, exclude, boost, demote, stats, config, quit" << endl;
            }
        }
    }
    
private:
    string generateSessionId() {
        auto now = chrono::system_clock::now();
        auto time_t = chrono::system_clock::to_time_t(now);
        stringstream ss;
        ss << "session_" << time_t;
        return ss.str();
    }
    
    vector<string> splitCommand(const string& input) {
        vector<string> parts;
        stringstream ss(input);
        string part;
        while (ss >> part) {
            parts.push_back(part);
        }
        return parts;
    }
    
    void displaySearchResults(const vector<SearchResult>& results) {
        if (results.empty()) {
            cout << "No results found." << endl;
            return;
        }
        
        cout << "\n--- SEARCH RESULTS ---" << endl;
        for (size_t i = 0; i < results.size(); ++i) {
            const auto& result = results[i];
            cout << (i + 1) << ". " << result.entry.path << endl;
            cout << "   Category: " << result.entry.category 
                 << " | Type: " << result.entry.fieldType << endl;
            cout << "   Score: " << fixed << setprecision(2) << result.finalScore
                 << " (Text: " << result.textScore << ", Vector: " << result.vectorScore << ")" << endl;
            cout << "   Explanation: " << result.explanation << endl;
            
            if (!result.matchReasons.empty()) {
                cout << "   Match reasons: ";
                for (size_t j = 0; j < result.matchReasons.size(); ++j) {
                    if (j > 0) cout << ", ";
                    cout << result.matchReasons[j];
                }
                cout << endl;
            }
            
            if (!result.relatedPaths.empty()) {
                cout << "   Related: ";
                for (size_t j = 0; j < min((size_t)3, result.relatedPaths.size()); ++j) {
                    if (j > 0) cout << ", ";
                    cout << result.relatedPaths[j];
                }
                cout << endl;
            }
            cout << endl;
        }
    }
    
    void printUserStats() {
        cout << "\n--- USER SESSION STATS ---" << endl;
        cout << "Session ID: " << context.sessionId << endl;
        cout << "Selected paths: " << context.selectedPaths.size() << endl;
        cout << "Excluded paths: " << context.excludedPaths.size() << endl;
        cout << "Search history: " << context.searchHistory.size() << endl;
        cout << "Learned preferences:" << endl;
        
        for (const auto& [category, preference] : context.preferences) {
            cout << "  " << category << ": " << fixed << setprecision(2) << preference << endl;
        }
    }
};

int main() {
    cout << "Pointing Index System - Advanced Configuration Search & Suggestion" << endl;
    cout << "=================================================================" << endl;
    
    try {
        PointingSession session;
        session.runInteractiveSession();
        
        cout << "\nSession ended. Thank you!" << endl;
        
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}