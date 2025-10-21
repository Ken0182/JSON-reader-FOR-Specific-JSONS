/**
 * @file seed_semantic_db.cpp
 * @brief Semantic Database Seeder - Extract & Embed Knowledge Base
 * @author AI Assistant
 * @version 1.0
 * 
 * Extracts tags, aliases, and descriptors from:
 * - All markdown files (*.md)
 * - Configuration JSON files (clean_config.json, group.json, guitar.json, structure.json)
 * - Generates embeddings using SentenceEncoder
 * - Populates semantic.db with tags, aliases, embeddings, and tunable parameters
 * 
 * Usage: ./seed_semantic_db [--db semantic.db] [--dimension 100]
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cmath>
#include <regex>

#include "../src/semantic_db.hpp"
#include "../src/semantic_knowledge_base.hpp"
#include "../src/sentence_encoder.hpp"
#include "../src/text_utils.hpp"
#include "../src/json.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;
using namespace audio_config;

// Tag extraction patterns
const std::regex MARKDOWN_TAG_PATTERN(R"(\b(warm|bright|dark|cold|soft|hard|smooth|rough|clean|dirty|fat|thin|thick|crisp|mellow|harsh|gentle|aggressive|calm|dreamy|lush|sparse|rich|full|empty|dense|airy|ethereal|organic|synthetic|natural|artificial|acoustic|electric|analog|digital|vintage|modern|retro|classic|old|new|fresh|stale|raw|processed|dry|wet|punchy|sustained|percussive|melodic|harmonic|rhythmic|ambient|atmospheric|spacious|tight|loose|dynamic|static|evolving|stable|chaotic|ordered|funky|groovy|bouncy|plucky|metallic|wooden|glassy|plastic|tribal|industrial|cosmic|infinite|intimate|bold|delicate|energetic|steady|driving|reflective|nostalgic|uplifting|hypnotic|unsettling|mysterious|futuristic|crystalline|jangly|fuzzy|crisp|squelchy|gritty|bell-like|plucky|experimental)\b)", std::regex::icase);

// Common alias mappings
const std::unordered_map<std::string, std::string> COMMON_ALIASES = {
    {"synthesizer", "synth"},
    {"synthesiser", "synth"},
    {"keyboard", "keys"},
    {"percussion", "perc"},
    {"arpeggiator", "arp"},
    {"reverberation", "reverb"},
    {"compressor", "comp"},
    {"equalizer", "eq"},
    {"lowpass", "low-pass"},
    {"highpass", "high-pass"},
    {"bandpass", "band-pass"},
};

// Default tunable parameters
const std::unordered_map<std::string, float> DEFAULT_TUNABLES = {
    {"semantic_weight", 0.4f},
    {"tag_weight", 0.3f},
    {"idf_lambda", 0.5f},
    {"embedding_weight", 0.7f},
    {"min_similarity_threshold", 0.3f},
    {"max_results", 10.0f},
    {"cosine_threshold", 0.5f},
};

/**
 * @brief Extract tags from markdown text
 */
std::unordered_set<std::string> extractTagsFromMarkdown(const std::string& content) {
    std::unordered_set<std::string> tags;
    
    auto words_begin = std::sregex_iterator(content.begin(), content.end(), MARKDOWN_TAG_PATTERN);
    auto words_end = std::sregex_iterator();
    
    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        std::smatch match = *i;
        std::string tag = TextUtils::toLower(match.str());
        tags.insert(tag);
    }
    
    return tags;
}

/**
 * @brief Extract tags from JSON configuration
 */
std::unordered_set<std::string> extractTagsFromJSON(const json& config) {
    std::unordered_set<std::string> tags;
    
    // Helper to recursively extract string values
    std::function<void(const json&)> extractStrings = [&](const json& j) {
        if (j.is_string()) {
            std::string val = j.get<std::string>();
            auto tokens = TextUtils::tokenize(val);
            for (const auto& token : tokens) {
                if (token.length() >= 3) {  // Skip very short tokens
                    tags.insert(token);
                }
            }
        } else if (j.is_object()) {
            // Extract from sound_characteristics, emotional tags, etc.
            if (j.contains("soundCharacteristics")) {
                extractStrings(j["soundCharacteristics"]);
            }
            if (j.contains("sound_characteristics")) {
                extractStrings(j["sound_characteristics"]);
            }
            
            for (auto& [key, value] : j.items()) {
                // Extract from keys like "timbral", "material", "emotional"
                if (key == "timbral" || key == "material" || key == "dynamic" ||
                    key == "emotional" || key == "tag") {
                    extractStrings(value);
                }
                
                // Recursively process nested objects
                if (value.is_object() || value.is_array()) {
                    extractStrings(value);
                }
            }
        } else if (j.is_array()) {
            for (const auto& item : j) {
                extractStrings(item);
            }
        }
    };
    
    extractStrings(config);
    return tags;
}

/**
 * @brief Main seeding function
 */
int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "Semantic Database Seeder v1.0" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Parse command line arguments
    std::string dbPath = "semantic.db";
    int dimension = 100;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--db" && i + 1 < argc) {
            dbPath = argv[++i];
        } else if (arg == "--dimension" && i + 1 < argc) {
            dimension = std::stoi(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --db <path>        Database path (default: semantic.db)" << std::endl;
            std::cout << "  --dimension <n>    Embedding dimension (default: 100)" << std::endl;
            std::cout << "  --help, -h         Show this help" << std::endl;
            return 0;
        }
    }
    
    std::cout << "Database: " << dbPath << std::endl;
    std::cout << "Dimension: " << dimension << std::endl;
    std::cout << std::endl;
    
    // Collect all tags
    std::unordered_set<std::string> allTags;
    std::unordered_map<std::string, std::string> aliases;
    
    // 1. Extract from MD files
    std::cout << "Extracting tags from Markdown files..." << std::endl;
    int mdCount = 0;
    for (const auto& entry : fs::recursive_directory_iterator(".")) {
        if (entry.is_regular_file() && entry.path().extension() == ".md") {
            std::ifstream file(entry.path());
            std::string content((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
            
            auto tags = extractTagsFromMarkdown(content);
            allTags.insert(tags.begin(), tags.end());
            mdCount++;
            
            std::cout << "  " << entry.path().filename() << ": " << tags.size() << " tags" << std::endl;
        }
    }
    std::cout << "Processed " << mdCount << " markdown files" << std::endl;
    std::cout << std::endl;
    
    // 2. Extract from JSON files
    std::cout << "Extracting tags from JSON configuration files..." << std::endl;
    std::vector<std::string> jsonFiles = {
        "clean_config.json",
        "data/clean_config.json",
        "group.json",
        "guitar.json",
        "structure.json"
    };
    
    int jsonCount = 0;
    for (const auto& jsonFile : jsonFiles) {
        if (fs::exists(jsonFile)) {
            std::ifstream file(jsonFile);
            json config;
            file >> config;
            
            auto tags = extractTagsFromJSON(config);
            allTags.insert(tags.begin(), tags.end());
            jsonCount++;
            
            std::cout << "  " << jsonFile << ": " << tags.size() << " tags" << std::endl;
        }
    }
    std::cout << "Processed " << jsonCount << " JSON files" << std::endl;
    std::cout << std::endl;
    
    // 3. Add common aliases
    for (const auto& [alias, canonical] : COMMON_ALIASES) {
        aliases[alias] = canonical;
        allTags.insert(canonical);  // Ensure canonical form exists
    }
    
    std::cout << "Total unique tags extracted: " << allTags.size() << std::endl;
    std::cout << "Total aliases defined: " << aliases.size() << std::endl;
    std::cout << std::endl;
    
    // 4. Initialize database
    std::cout << "Initializing semantic database..." << std::endl;
    try {
        // Remove existing database
        if (fs::exists(dbPath)) {
            fs::remove(dbPath);
            std::cout << "Removed existing database" << std::endl;
        }
        
        SemanticDatabase db(dbPath);
        if (!db.initializeSchema()) {
            std::cerr << "Failed to initialize database schema" << std::endl;
            return 1;
        }
        std::cout << "Database schema initialized" << std::endl;
        std::cout << std::endl;
        
        // 5. Create sentence encoder
        std::cout << "Creating sentence encoder..." << std::endl;
        auto encoder = SentenceEncoder::createHashEncoder(&db, dimension);
        if (!encoder || !encoder->isReady()) {
            std::cerr << "Failed to create sentence encoder" << std::endl;
            return 1;
        }
        std::cout << "Sentence encoder ready (dimension: " << dimension << ")" << std::endl;
        std::cout << std::endl;
        
        // 6. Generate embeddings and populate database
        std::cout << "Generating embeddings and populating database..." << std::endl;
        std::cout << "Progress:" << std::endl;
        
        int count = 0;
        int total = allTags.size();
        std::vector<std::string> allTagsList(allTags.begin(), allTags.end());
        std::sort(allTagsList.begin(), allTagsList.end());
        
        for (const auto& tag : allTagsList) {
            // Generate embedding
            auto embedding = encoder->encode(tag);
            
            // Verify unit norm
            float norm = 0.0f;
            for (float val : embedding) {
                norm += val * val;
            }
            norm = std::sqrt(norm);
            
            if (std::abs(norm - 1.0f) > 0.01f) {
                std::cerr << "Warning: Non-unit norm for tag '" << tag << "': " << norm << std::endl;
            }
            
            // Store in database
            std::string canonical = tag;
            if (!db.storeEmbedding(tag, embedding, canonical)) {
                std::cerr << "Failed to store embedding for: " << tag << std::endl;
            }
            
            count++;
            if (count % 10 == 0 || count == total) {
                std::cout << "  " << count << "/" << total << " embeddings generated" << std::endl;
            }
        }
        
        std::cout << "All embeddings generated and stored" << std::endl;
        std::cout << std::endl;
        
        // 7. Store aliases
        std::cout << "Storing aliases..." << std::endl;
        for (const auto& [alias, canonical] : aliases) {
            if (allTags.count(canonical) > 0) {
                // Store alias with canonical mapping
                auto embedding = encoder->encode(alias);
                db.storeEmbedding(alias, embedding, canonical);
            }
        }
        std::cout << "Stored " << aliases.size() << " aliases" << std::endl;
        std::cout << std::endl;
        
        // 8. Store tunable parameters
        std::cout << "Storing tunable parameters..." << std::endl;
        for (const auto& [key, value] : DEFAULT_TUNABLES) {
            if (!db.storeConfig(key, value)) {
                std::cerr << "Failed to store config: " << key << std::endl;
            }
        }
        std::cout << "Stored " << DEFAULT_TUNABLES.size() << " tunable parameters" << std::endl;
        std::cout << std::endl;
        
        // 9. Compute IDF statistics (simple uniform for now)
        std::cout << "Computing IDF statistics..." << std::endl;
        int idfCount = 0;
        for (const auto& tag : allTagsList) {
            // Simple uniform IDF for now
            float idf = 1.0f;  // Will be updated with real corpus statistics
            int docCount = 1;
            
            if (db.storeIDF(tag, idf, docCount)) {
                idfCount++;
            }
        }
        std::cout << "Computed IDF for " << idfCount << " tags" << std::endl;
        std::cout << std::endl;
        
        // 10. Final summary
        std::cout << "========================================" << std::endl;
        std::cout << "Seeding Complete!" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Database: " << dbPath << std::endl;
        std::cout << "  Tags:        " << allTags.size() << std::endl;
        std::cout << "  Aliases:     " << aliases.size() << std::endl;
        std::cout << "  Embeddings:  " << count << std::endl;
        std::cout << "  Dimension:   " << dimension << std::endl;
        std::cout << "  Tunables:    " << DEFAULT_TUNABLES.size() << std::endl;
        std::cout << "  IDF stats:   " << idfCount << std::endl;
        std::cout << std::endl;
        
        // Verification
        std::cout << "Verification:" << std::endl;
        std::cout << "  ✓ Database file created: " << fs::file_size(dbPath) << " bytes" << std::endl;
        
        // Sample tags
        std::vector<std::string> sampleTags;
        int sampleCount = std::min(5, (int)allTagsList.size());
        for (int i = 0; i < sampleCount; ++i) {
            sampleTags.push_back(allTagsList[i]);
        }
        std::cout << "  ✓ Sample tags: ";
        for (size_t i = 0; i < sampleTags.size(); ++i) {
            std::cout << sampleTags[i];
            if (i < sampleTags.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
        
        std::cout << std::endl;
        std::cout << "Database ready for use!" << std::endl;
        std::cout << "Run with: ./audio_config_system --db " << dbPath << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
