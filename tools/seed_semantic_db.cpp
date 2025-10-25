/**
 * @file seed_semantic_db.cpp
 * @brief Semantic Knowledge Base Seeder
 * @author AI Assistant
 * @version 1.0
 * 
 * Seeds SQLite database with tags, aliases, embeddings, and tunable knobs
 * from all JSON configuration files in the repository.
 * 
 * USAGE:
 *   ./seed_semantic_db [database_path] [--force]
 *   
 *   database_path: Path to SQLite database (default: semantic.db)
 *   --force: Overwrite existing database
 * 
 * FEATURES:
 * - Extracts tags from all JSON files (clean_config.json, group.json, guitar.json, structure.json)
 * - Generates embeddings using sentence encoder
 * - Creates canonical tag mappings and aliases
 * - Computes IDF statistics from tag corpus
 * - Stores tunable parameters in config table
 * - Validates database integrity
 */

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cmath>
#include <cassert>

// Include our semantic database and encoder
#include "../src/semantic_db.hpp"
#include "../src/semantic_knowledge_base.hpp"
#include "../src/sentence_encoder.hpp"
#include "../src/text_utils.hpp"

// Include JSON parser
#include "../json.hpp"

using json = nlohmann::json;

namespace audio_config {

/**
 * @brief Tag extraction and seeding utility
 */
class SemanticSeeder {
public:
    explicit SemanticSeeder(const std::string& dbPath) 
        : dbPath_(dbPath), kb_(nullptr) {}
    
    /**
     * @brief Run complete seeding process
     * @param force Overwrite existing database
     * @return true if successful
     */
    bool seed(bool force = false);
    
    /**
     * @brief Print database statistics
     */
    void printStats();
    
private:
    std::string dbPath_;
    std::unique_ptr<SemanticKnowledgeBase> kb_;
    std::unordered_set<std::string> allTags_;
    std::unordered_map<std::string, std::string> aliases_;
    
    /**
     * @brief Extract tags from JSON file
     * @param filePath Path to JSON file
     * @return Number of tags extracted
     */
    int extractTagsFromFile(const std::string& filePath);
    
    /**
     * @brief Extract tags from JSON object recursively
     * @param obj JSON object to traverse
     * @param path Current path in JSON (for context)
     * @return Number of tags extracted
     */
    int extractTagsFromObject(const json& obj, const std::string& path = "");
    
    /**
     * @brief Extract tags from string value
     * @param value String value
     * @param context Context for the tag
     * @return Vector of extracted tags
     */
    std::vector<std::string> extractTagsFromString(const std::string& value, const std::string& context = "");
    
    /**
     * @brief Create canonical tag mappings and aliases
     */
    void createAliases();
    
    /**
     * @brief Generate embeddings for all tags
     * @return Number of embeddings generated
     */
    int generateEmbeddings();
    
    /**
     * @brief Compute and store IDF statistics
     * @return Number of IDF entries computed
     */
    int computeIDFStatistics();
    
    /**
     * @brief Store tunable parameters
     * @return Number of parameters stored
     */
    int storeTunableParameters();
    
    /**
     * @brief Validate database integrity
     * @return true if valid
     */
    bool validateDatabase();
};

bool SemanticSeeder::seed(bool force) {
    std::cout << "=== Semantic Knowledge Base Seeder ===" << std::endl;
    std::cout << "Database: " << dbPath_ << std::endl;
    
    // Check if database exists
    if (std::filesystem::exists(dbPath_) && !force) {
        std::cout << "Database already exists. Use --force to overwrite." << std::endl;
        return false;
    }
    
    // Remove existing database if force
    if (force && std::filesystem::exists(dbPath_)) {
        std::filesystem::remove(dbPath_);
        std::cout << "Removed existing database." << std::endl;
    }
    
    // Create knowledge base
    try {
        kb_ = std::make_unique<SemanticKnowledgeBase>(dbPath_);
        if (!kb_->initialize(true)) {
            std::cerr << "Failed to initialize knowledge base" << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to create knowledge base: " << e.what() << std::endl;
        return false;
    }
    
    std::cout << "Knowledge base initialized with dimension: " << kb_->getDimension() << std::endl;
    
    // Extract tags from all JSON files
    std::vector<std::string> jsonFiles = {
        "clean_config.json",
        "group.json", 
        "guitar.json",
        "structure.json"
    };
    
    int totalTags = 0;
    for (const auto& file : jsonFiles) {
        if (std::filesystem::exists(file)) {
            std::cout << "Extracting tags from " << file << "..." << std::endl;
            int count = extractTagsFromFile(file);
            totalTags += count;
            std::cout << "  Extracted " << count << " tags" << std::endl;
        } else {
            std::cout << "Warning: " << file << " not found, skipping" << std::endl;
        }
    }
    
    std::cout << "Total unique tags extracted: " << allTags_.size() << std::endl;
    
    // Create aliases and canonical mappings
    std::cout << "Creating aliases..." << std::endl;
    createAliases();
    
    // Generate embeddings
    std::cout << "Generating embeddings..." << std::endl;
    int embedCount = generateEmbeddings();
    std::cout << "Generated " << embedCount << " embeddings" << std::endl;
    
    // Compute IDF statistics
    std::cout << "Computing IDF statistics..." << std::endl;
    int idfCount = computeIDFStatistics();
    std::cout << "Computed " << idfCount << " IDF entries" << std::endl;
    
    // Store tunable parameters
    std::cout << "Storing tunable parameters..." << std::endl;
    int paramCount = storeTunableParameters();
    std::cout << "Stored " << paramCount << " parameters" << std::endl;
    
    // Validate database
    std::cout << "Validating database..." << std::endl;
    if (!validateDatabase()) {
        std::cerr << "Database validation failed!" << std::endl;
        return false;
    }
    
    std::cout << "=== Seeding Complete ===" << std::endl;
    printStats();
    
    return true;
}

int SemanticSeeder::extractTagsFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open " << filePath << std::endl;
        return 0;
    }
    
    json data;
    try {
        file >> data;
    } catch (const json::exception& e) {
        std::cerr << "JSON parse error in " << filePath << ": " << e.what() << std::endl;
        return 0;
    }
    
    return extractTagsFromObject(data, filePath);
}

int SemanticSeeder::extractTagsFromObject(const json& obj, const std::string& path) {
    int count = 0;
    
    if (obj.is_object()) {
        for (auto& [key, value] : obj.items()) {
            std::string newPath = path.empty() ? key : path + "." + key;
            
            // Extract tags from key
            auto keyTags = extractTagsFromString(key, "key");
            for (const auto& tag : keyTags) {
                allTags_.insert(tag);
                count++;
            }
            
            // Recursively process value
            count += extractTagsFromObject(value, newPath);
        }
    } else if (obj.is_array()) {
        for (size_t i = 0; i < obj.size(); ++i) {
            std::string newPath = path + "[" + std::to_string(i) + "]";
            count += extractTagsFromObject(obj[i], newPath);
        }
    } else if (obj.is_string()) {
        auto tags = extractTagsFromString(obj.get<std::string>(), path);
        for (const auto& tag : tags) {
            allTags_.insert(tag);
            count++;
        }
    }
    
    return count;
}

std::vector<std::string> SemanticSeeder::extractTagsFromString(const std::string& value, const std::string& /* context */) {
    std::vector<std::string> tags;
    
    // Skip very short strings and numbers
    if (value.length() < 2) return tags;
    
    // Skip pure numbers
    if (std::all_of(value.begin(), value.end(), ::isdigit)) return tags;
    
    // Skip common non-semantic values
    static const std::unordered_set<std::string> skipValues = {
        "type", "enabled", "ai_control", "mix", "wet", "gain", "amount", "rate", "depth",
        "feedback", "time", "decay", "attack", "release", "sustain", "hold", "delay",
        "curve", "slope", "cutoff", "resonance", "threshold", "ratio", "probability",
        "intensity", "burst_length", "noise_type", "noiseIntensity", "noiseProbability",
        "position", "stiffness", "gauge", "tension", "num_strings", "num_layers",
        "randomize_range", "volume", "harmonics", "vibe_set", "decay_rate",
        "sympathetic_resonance", "freq_range", "vibrato_hz", "depth_cents",
        "detune_range", "tuning", "ir_file", "table_index", "morph_rate",
        "grain_density", "grain_size", "pluck_position", "blend_mode",
        "carrier_ratio", "modulation_index", "envelope_amount", "gateDecaySec",
        "gateThreshold", "useDynamicGate", "attackMul", "decayMul", "sustainMul",
        "releaseMul", "sectionName", "ai_control", "ai_dynamic", "automated",
        "random", "variable", "unstable", "extreme", "very", "high", "low", "medium"
    };
    
    if (skipValues.find(value) != skipValues.end()) return tags;
    
    // Tokenize the string
    auto tokens = TextUtils::tokenize(value);
    
    // Filter out very common words and short tokens
    static const std::unordered_set<std::string> commonWords = {
        "a", "an", "the", "and", "or", "but", "in", "on", "at", "to", "for", "of", "with",
        "by", "from", "up", "about", "into", "through", "during", "before", "after",
        "above", "below", "between", "among", "is", "are", "was", "were", "be", "been",
        "being", "have", "has", "had", "do", "does", "did", "will", "would", "could",
        "should", "may", "might", "must", "can", "shall", "it", "its", "this", "that",
        "these", "those", "i", "you", "he", "she", "we", "they", "me", "him", "her",
        "us", "them", "my", "your", "his", "her", "our", "their", "mine", "yours",
        "hers", "ours", "theirs"
    };
    
    for (const auto& token : tokens) {
        if (token.length() >= 2 && commonWords.find(token) == commonWords.end()) {
            tags.push_back(token);
        }
    }
    
    return tags;
}

void SemanticSeeder::createAliases() {
    // Create aliases for common synonyms and variations
    aliases_ = {
        // Timbre descriptors
        {"bright", "bright"},
        {"brilliant", "bright"},
        {"crisp", "bright"},
        {"sharp", "bright"},
        {"clear", "bright"},
        
        {"warm", "warm"},
        {"soft", "warm"},
        {"smooth", "warm"},
        {"mellow", "warm"},
        {"round", "warm"},
        
        {"dark", "dark"},
        {"deep", "dark"},
        {"heavy", "dark"},
        {"thick", "dark"},
        {"shadowy", "dark"},
        
        // Synthesis types
        {"analog", "analog"},
        {"vintage", "analog"},
        {"classic", "analog"},
        {"retro", "analog"},
        
        {"digital", "digital"},
        {"clean", "digital"},
        {"precise", "digital"},
        {"modern", "digital"},
        
        // Dynamic descriptors
        {"punchy", "punchy"},
        {"sharp", "punchy"},
        {"attack", "punchy"},
        {"percussive", "punchy"},
        {"dynamic", "punchy"},
        
        {"fat", "fat"},
        {"thick", "fat"},
        {"heavy", "fat"},
        {"full", "fat"},
        {"rich", "fat"},
        
        {"thin", "thin"},
        {"light", "thin"},
        {"airy", "thin"},
        {"sparse", "thin"},
        {"minimal", "thin"},
        
        // Emotional descriptors
        {"aggressive", "aggressive"},
        {"harsh", "aggressive"},
        {"intense", "aggressive"},
        {"powerful", "aggressive"},
        {"bold", "aggressive"},
        
        {"dreamy", "dreamy"},
        {"ethereal", "dreamy"},
        {"floating", "dreamy"},
        {"ambient", "dreamy"},
        
        {"lush", "lush"},
        {"rich", "lush"},
        {"full", "lush"},
        {"dense", "lush"},
        {"layered", "lush"},
        
        // Material descriptors
        {"organic", "organic"},
        {"natural", "organic"},
        {"acoustic", "organic"},
        {"real", "organic"},
        
        {"synthetic", "synthetic"},
        {"artificial", "synthetic"},
        {"processed", "synthetic"},
        {"electronic", "synthetic"},
        
        {"metallic", "metallic"},
        {"cold", "metallic"},
        
        {"wooden", "wooden"},
        {"wood", "wooden"},
        
        // New adjectives from JSON files
        {"fuzzy", "fuzzy"},
        {"jangly", "jangly"},
        {"crisp", "crisp"},
        {"gritty", "gritty"},
        {"squelchy", "squelchy"},
        {"glassy", "glassy"},
        {"crystalline", "crystalline"},
        {"plucky", "plucky"},
        {"chaotic", "chaotic"},
        {"experimental", "experimental"},
        {"unsettling", "unsettling"},
        {"mystical", "mystical"},
        {"futuristic", "futuristic"},
        {"cosmic", "cosmic"},
        {"infinite", "infinite"},
        {"evolving", "evolving"},
        {"unstable", "unstable"},
        {"intense", "intense"},
        {"energetic", "energetic"},
        {"uplifting", "uplifting"},
        {"nostalgic", "nostalgic"},
        {"delicate", "delicate"},
        {"reflective", "reflective"},
        {"intimate", "intimate"},
        {"expressive", "expressive"},
        {"tight", "tight"},
        {"damped", "damped"},
        {"muted", "muted"},
        {"sour", "sour"},
        {"shiny", "shiny"},
        {"tribal", "tribal"},
        {"perturbance", "perturbance"},
        {"riveting", "riveting"},
        {"jittery", "jittery"},
        {"playful", "playful"},
        {"rhythmic", "rhythmic"},
        {"bouncy", "bouncy"},
        {"solid", "solid"},
        {"driving", "driving"},
        {"steady", "steady"},
        {"funky", "funky"},
        {"hypnotic", "hypnotic"},
        {"development", "development"},
        {"surprising", "surprising"},
        {"vintage", "vintage"},
        {"calm", "calm"},
        {"reflective", "reflective"}
    };
    
    // Store aliases in database
    for (const auto& [alias, canonical] : aliases_) {
        if (allTags_.find(alias) != allTags_.end()) {
            // Generate embedding for alias
            auto embedding = kb_->encodeText(alias);
            kb_->storeTagEmbedding(alias, embedding, canonical);
        }
    }
}

int SemanticSeeder::generateEmbeddings() {
    int count = 0;
    
    for (const auto& tag : allTags_) {
        // Check if we have a canonical form
        std::string canonical = tag;
        if (aliases_.find(tag) != aliases_.end()) {
            canonical = aliases_[tag];
        }
        
        // Generate embedding
        auto embedding = kb_->encodeText(tag);
        if (!embedding.empty()) {
            if (kb_->storeTagEmbedding(tag, embedding, canonical)) {
                count++;
            }
        }
    }
    
    return count;
}

int SemanticSeeder::computeIDFStatistics() {
    // Treat each source file as a document: for simplicity, split unique tags arbitrarily into one doc
    // Here we create a single document containing all tags, but callers may pass structured docs.
    std::vector<std::vector<std::string>> docs;
    docs.emplace_back(allTags_.begin(), allTags_.end());
    return kb_->computeIDFStatistics(docs);
}

int SemanticSeeder::storeTunableParameters() {
    int count = 0;
    
    // Store common tunable parameters
    std::vector<std::pair<std::string, float>> params = {
        {"semantic_weight", 0.7f},
        {"alpha", 0.5f},
        {"beta", 0.3f},
        {"gamma", 0.2f},
        {"similarity_threshold", 0.6f},
        {"max_results", 10.0f},
        {"embedding_dimension", static_cast<float>(kb_->getDimension())},
        {"idf_weight", 0.4f},
        {"canonical_weight", 0.8f},
        {"query_expansion", 0.3f}
    };
    
    for (const auto& [key, value] : params) {
        if (kb_->storeConfig(key, value)) {
            count++;
        }
    }
    
    return count;
}

bool SemanticSeeder::validateDatabase() {
    // Check if database is valid
    if (!kb_->isReady()) {
        std::cerr << "Knowledge base not ready" << std::endl;
        return false;
    }
    
    // Check dimension consistency
    int dimension = kb_->getDimension();
    if (dimension <= 0) {
        std::cerr << "Invalid embedding dimension: " << dimension << std::endl;
        return false;
    }
    
    // Check that we have embeddings
    auto allTags = kb_->getAllTags();
    if (allTags.empty()) {
        std::cerr << "No tags found in database" << std::endl;
        return false;
    }
    
    // Verify some embeddings are unit-norm
    int checked = 0;
    int unitNorm = 0;
    for (const auto& tag : allTags) {
        if (checked >= 10) break; // Check first 10 tags
        
        auto embedding = kb_->getTagEmbedding(tag);
        if (!embedding.empty()) {
            // Check if unit-norm (within tolerance)
            float norm = 0.0f;
            for (float val : embedding) {
                norm += val * val;
            }
            norm = std::sqrt(norm);
            
            if (std::abs(norm - 1.0f) < 0.01f) {
                unitNorm++;
            }
            checked++;
        }
    }
    
    if (checked > 0 && unitNorm == checked) {
        std::cout << "All checked embeddings are unit-norm ✓" << std::endl;
    } else {
        std::cout << "Warning: Some embeddings may not be unit-norm" << std::endl;
    }
    
    return true;
}

void SemanticSeeder::printStats() {
    if (!kb_) {
        std::cout << "Knowledge base not initialized" << std::endl;
        return;
    }
    
    auto allTags = kb_->getAllTags();
    
    std::cout << "\n=== Database Statistics ===" << std::endl;
    std::cout << "Dimension: " << kb_->getDimension() << std::endl;
    std::cout << "Tag count: " << allTags.size() << std::endl;
    
    // Count aliases
    int aliasCount = 0;
    for (const auto& tag : allTags) {
        std::string canonical = kb_->getCanonicalTag(tag);
        if (canonical != tag) {
            aliasCount++;
        }
    }
    std::cout << "Alias count: " << aliasCount << std::endl;
    
    // Show sample tags
    std::cout << "Sample tags: ";
    int shown = 0;
    for (const auto& tag : allTags) {
        if (shown >= 10) break;
        std::cout << tag;
        if (shown < 9 && shown < static_cast<int>(allTags.size()) - 1) {
            std::cout << ", ";
        }
        shown++;
    }
    std::cout << std::endl;
    
    // Test search functionality
    std::cout << "\n=== Search Test ===" << std::endl;
    std::string testQuery = "dreamy not harsh";
    std::cout << "Query: \"" << testQuery << "\"" << std::endl;
    
    auto queryVec = kb_->encodeText(testQuery);
    if (!queryVec.empty()) {
        std::cout << "Query vector dimension: " << queryVec.size() << std::endl;
        
        // Find most similar tags
        std::vector<std::pair<std::string, float>> similarities;
        for (const auto& tag : allTags) {
            auto tagVec = kb_->getTagEmbedding(tag);
            if (!tagVec.empty()) {
                float sim = SemanticKnowledgeBase::cosineSimilarity(queryVec, tagVec);
                similarities.push_back({tag, sim});
            }
        }
        
        // Sort by similarity
        std::sort(similarities.begin(), similarities.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        std::cout << "Top matches:" << std::endl;
        for (int i = 0; i < std::min(5, static_cast<int>(similarities.size())); ++i) {
            std::cout << "  " << (i+1) << ". " << similarities[i].first 
                      << " (similarity: " << similarities[i].second << ")" << std::endl;
        }
    } else {
        std::cout << "Failed to encode query" << std::endl;
    }
}

} // namespace audio_config

int main(int argc, char* argv[]) {
    std::string dbPath = "semantic.db";
    bool force = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--force") {
            force = true;
        } else if (arg[0] != '-') {
            dbPath = arg;
        }
    }
    
    try {
        audio_config::SemanticSeeder seeder(dbPath);
        if (seeder.seed(force)) {
            std::cout << "Seeding completed successfully!" << std::endl;
            return 0;
        } else {
            std::cerr << "Seeding failed!" << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
