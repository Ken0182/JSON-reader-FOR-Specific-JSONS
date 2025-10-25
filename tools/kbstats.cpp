/**
 * @file kbstats.cpp
 * @brief Knowledge Base Statistics and Integrity Checker
 * @author AI Assistant
 * @version 1.0
 * 
 * Prints database statistics and validates integrity of semantic knowledge base.
 * 
 * USAGE:
 *   ./kbstats [database_path]
 *   
 *   database_path: Path to SQLite database (default: semantic.db)
 * 
 * OUTPUT:
 *   - Dimension (D)
 *   - Tag count
 *   - Alias count  
 *   - Sample tags
 *   - Vector normalization check
 *   - Search test results
 */

#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <fstream>

// Include our semantic database and encoder
#include "../src/semantic_db.hpp"
#include "../src/semantic_knowledge_base.hpp"
#include "../json.hpp"

namespace audio_config {

/**
 * @brief Knowledge base statistics and validation utility
 */
class KBStats {
public:
    explicit KBStats(const std::string& dbPath) 
        : dbPath_(dbPath), kb_(nullptr) {}
    
    /**
     * @brief Load knowledge base and print statistics
     * @return true if successful
     */
    bool printStats();
    bool refresh();
    
private:
    std::string dbPath_;
    std::unique_ptr<SemanticKnowledgeBase> kb_;
    
    /**
     * @brief Validate vector normalization
     * @return Number of vectors checked and number that are unit-norm
     */
    std::pair<int, int> validateNormalization();
    
    /**
     * @brief Test search functionality
     * @param query Test query string
     * @return true if search works
     */
    bool testSearch(const std::string& query);
    
    /**
     * @brief Print detailed tag information
     */
    void printTagDetails();
};

bool KBStats::printStats() {
    std::cout << "=== Knowledge Base Statistics ===" << std::endl;
    std::cout << "Database: " << dbPath_ << std::endl;
    
    // Load knowledge base
    try {
        kb_ = std::make_unique<SemanticKnowledgeBase>(dbPath_);
        if (!kb_->initialize()) {
            std::cerr << "Failed to initialize knowledge base" << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to load knowledge base: " << e.what() << std::endl;
        return false;
    }
    
    // Basic statistics
    int dimension = kb_->getDimension();
    auto allTags = kb_->getAllTags();
    int tagCount = static_cast<int>(allTags.size());
    
    std::cout << "\n--- Basic Statistics ---" << std::endl;
    std::cout << "Dimension (D): " << dimension << std::endl;
    std::cout << "Tag count: " << tagCount << std::endl;
    
    // Count aliases
    int aliasCount = 0;
    for (const auto& tag : allTags) {
        std::string canonical = kb_->getCanonicalTag(tag);
        if (canonical != tag) {
            aliasCount++;
        }
    }
    std::cout << "Alias count: " << aliasCount << std::endl;
    
    // Sample tags
    std::cout << "\n--- Sample Tags ---" << std::endl;
    int shown = 0;
    for (const auto& tag : allTags) {
        if (shown >= 15) break;
        std::cout << "  " << tag;
        if (shown < 14 && shown < tagCount - 1) {
            std::cout << ", ";
        }
        if ((shown + 1) % 5 == 0) {
            std::cout << std::endl;
        }
        shown++;
    }
    if (shown % 5 != 0) {
        std::cout << std::endl;
    }
    
    // Vector normalization check
    std::cout << "\n--- Vector Normalization Check ---" << std::endl;
    auto [checked, unitNorm] = validateNormalization();
    std::cout << "Vectors checked: " << checked << std::endl;
    std::cout << "Unit-norm vectors: " << unitNorm << std::endl;
    if (checked > 0) {
        double percentage = (static_cast<double>(unitNorm) / checked) * 100.0;
        std::cout << "Normalization rate: " << std::fixed << std::setprecision(1) 
                  << percentage << "%" << std::endl;
    }
    
    // Search test
    std::cout << "\n--- Search Test ---" << std::endl;
    std::string testQuery = "dreamy not harsh";
    std::cout << "Query: \"" << testQuery << "\"" << std::endl;
    
    if (testSearch(testQuery)) {
        std::cout << "Search test: PASSED ✓" << std::endl;
    } else {
        std::cout << "Search test: FAILED ✗" << std::endl;
    }
    
    // Detailed tag information
    printTagDetails();
    
    return true;
}

bool KBStats::refresh() {
    std::cout << "=== Knowledge Base Sync ===" << std::endl;
    std::cout << "Database: " << dbPath_ << std::endl;
    try {
        kb_ = std::make_unique<SemanticKnowledgeBase>(dbPath_);
        if (!kb_->initialize()) {
            std::cerr << "Failed to initialize knowledge base" << std::endl;
            return false;
        }
        // Load clean_config.json to recompute IDF using per-document tags
        nlohmann::json configs;
        std::ifstream f("data/clean_config.json");
        if (!f.is_open()) {
            std::cerr << "Could not open data/clean_config.json to build corpus" << std::endl;
            return false;
        }
        f >> configs;
        std::vector<std::vector<std::string>> docs;
        for (auto& [id, cfg] : configs.items()) {
            (void)id;
            std::vector<std::string> tags;
            if (cfg.contains("soundCharacteristics")) {
                const auto& sc = cfg["soundCharacteristics"];
                if (sc.contains("timbral") && sc["timbral"].is_string()) {
                    tags.push_back(sc["timbral" ].get<std::string>());
                }
                if (sc.contains("dynamic") && sc["dynamic"].is_string()) {
                    tags.push_back(sc["dynamic" ].get<std::string>());
                }
                if (sc.contains("material") && sc["material"].is_string()) {
                    tags.push_back(sc["material" ].get<std::string>());
                }
                if (sc.contains("emotional") && sc["emotional"].is_array()) {
                    for (const auto& e : sc["emotional"]) {
                        if (e.is_object() && e.contains("tag")) tags.push_back(e["tag"].get<std::string>());
                    }
                }
            }
            docs.push_back(std::move(tags));
        }
        int n = kb_->computeIDFStatistics(docs);
        std::cout << "Recomputed IDF entries: " << n << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Sync error: " << e.what() << std::endl;
        return false;
    }
}

std::pair<int, int> KBStats::validateNormalization() {
    if (!kb_) return {0, 0};
    
    auto allTags = kb_->getAllTags();
    int checked = 0;
    int unitNorm = 0;
    
    // Check first 20 tags for normalization
    for (const auto& tag : allTags) {
        if (checked >= 20) break;
        
        auto embedding = kb_->getTagEmbedding(tag);
        if (!embedding.empty()) {
            // Calculate L2 norm
            float norm = 0.0f;
            for (float val : embedding) {
                norm += val * val;
            }
            norm = std::sqrt(norm);
            
            // Check if unit-norm (within tolerance)
            if (std::abs(norm - 1.0f) < 0.01f) {
                unitNorm++;
            }
            
            checked++;
        }
    }
    
    return {checked, unitNorm};
}

bool KBStats::testSearch(const std::string& query) {
    if (!kb_) return false;
    
    try {
        // Encode query
        auto queryVec = kb_->encodeText(query);
        if (queryVec.empty()) {
            std::cout << "  Failed to encode query" << std::endl;
            return false;
        }
        
        std::cout << "  Query vector dimension: " << queryVec.size() << std::endl;
        
        // Find most similar tags
        auto allTags = kb_->getAllTags();
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
        
        std::cout << "  Top matches:" << std::endl;
        for (int i = 0; i < std::min(5, static_cast<int>(similarities.size())); ++i) {
            std::cout << "    " << (i+1) << ". " << similarities[i].first 
                      << " (similarity: " << std::fixed << std::setprecision(3) 
                      << similarities[i].second << ")" << std::endl;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cout << "  Search error: " << e.what() << std::endl;
        return false;
    }
}

void KBStats::printTagDetails() {
    if (!kb_) return;
    
    auto allTags = kb_->getAllTags();
    
    std::cout << "\n--- Tag Details ---" << std::endl;
    
    // Count tags by category
    std::unordered_map<std::string, int> categories;
    for (const auto& tag : allTags) {
        // Simple categorization based on common patterns
        if (tag.find("bright") != std::string::npos || 
            tag.find("crisp") != std::string::npos ||
            tag.find("sharp") != std::string::npos) {
            categories["bright"]++;
        } else if (tag.find("warm") != std::string::npos || 
                   tag.find("soft") != std::string::npos ||
                   tag.find("smooth") != std::string::npos) {
            categories["warm"]++;
        } else if (tag.find("dark") != std::string::npos || 
                   tag.find("deep") != std::string::npos ||
                   tag.find("heavy") != std::string::npos) {
            categories["dark"]++;
        } else if (tag.find("aggressive") != std::string::npos || 
                   tag.find("harsh") != std::string::npos ||
                   tag.find("intense") != std::string::npos) {
            categories["aggressive"]++;
        } else if (tag.find("dreamy") != std::string::npos || 
                   tag.find("ethereal") != std::string::npos ||
                   tag.find("ambient") != std::string::npos) {
            categories["dreamy"]++;
        } else {
            categories["other"]++;
        }
    }
    
    std::cout << "Tag categories:" << std::endl;
    for (const auto& [category, count] : categories) {
        std::cout << "  " << category << ": " << count << std::endl;
    }
    
    // Show some aliases
    std::cout << "\nSample aliases:" << std::endl;
    int aliasShown = 0;
    for (const auto& tag : allTags) {
        if (aliasShown >= 10) break;
        
        std::string canonical = kb_->getCanonicalTag(tag);
        if (canonical != tag) {
            std::cout << "  " << tag << " → " << canonical << std::endl;
            aliasShown++;
        }
    }
    
    if (aliasShown == 0) {
        std::cout << "  No aliases found" << std::endl;
    }
}

} // namespace audio_config

int main(int argc, char* argv[]) {
    std::string dbPath = "semantic.db";
    std::string subcommand;

    // Parse command line: kbstats [db] [subcommand]
    if (argc > 1) {
        std::string arg1 = argv[1];
        if (!arg1.empty() && arg1[0] != '-') {
            dbPath = arg1;
            if (argc > 2) subcommand = argv[2];
        } else {
            subcommand = arg1;
        }
    }

    try {
        audio_config::KBStats stats(dbPath);
        if (subcommand == "refresh" || subcommand == "sync") {
            if (stats.refresh()) {
                std::cout << "\n=== Sync Complete ===" << std::endl;
                return 0;
            }
            std::cerr << "Sync failed" << std::endl;
            return 2;
        }
        if (stats.printStats()) {
            std::cout << "\n=== Statistics Complete ===" << std::endl;
            return 0;
        } else {
            std::cerr << "Failed to print statistics!" << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
