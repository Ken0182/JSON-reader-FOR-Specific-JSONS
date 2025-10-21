/**
 * @file kbstats.cpp
 * @brief Knowledge Base Statistics Tool
 * @brief Prints database integrity check: dimension, tag count, alias count, sample tags
 */

#include <iostream>
#include <iomanip>
#include <cmath>
#include "../src/semantic_db.hpp"
#include "../src/semantic_knowledge_base.hpp"

using namespace audio_config;

int main(int argc, char* argv[]) {
    std::string dbPath = "semantic.db";
    if (argc > 1) {
        dbPath = argv[1];
    }
    
    std::cout << "Knowledge Base Statistics" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "Database: " << dbPath << std::endl;
    std::cout << std::endl;
    
    try {
        // Open database
        SemanticDatabase db(dbPath);
        if (!db.isValid()) {
            std::cerr << "Error: Database not initialized or invalid" << std::endl;
            return 1;
        }
        
        // Get dimension
        int dimension = db.getEmbeddingDimension();
        std::cout << "Dimension (D): " << dimension << std::endl;
        
        // Get all tags
        auto allTags = db.getAllTags();
        std::cout << "Tag Count: " << allTags.size() << std::endl;
        
        // Count aliases (tags with different canonical forms)
        int aliasCount = 0;
        for (const auto& tag : allTags) {
            std::string canonical = db.getCanonicalTag(tag);
            if (canonical != tag) {
                aliasCount++;
            }
        }
        std::cout << "Alias Count: " << aliasCount << std::endl;
        
        // Sample tags
        std::cout << "\nSample Tags (first 10):" << std::endl;
        int sampleCount = std::min(10, static_cast<int>(allTags.size()));
        for (int i = 0; i < sampleCount; ++i) {
            std::cout << "  - " << allTags[i];
            
            // Show if it's an alias
            std::string canonical = db.getCanonicalTag(allTags[i]);
            if (canonical != allTags[i]) {
                std::cout << " (alias of '" << canonical << "')";
            }
            
            // Show norm
            auto embedding = db.getEmbedding(allTags[i]);
            if (!embedding.empty()) {
                float norm = 0.0f;
                for (float val : embedding) {
                    norm += val * val;
                }
                norm = std::sqrt(norm);
                std::cout << " |v|=" << std::fixed << std::setprecision(3) << norm;
            }
            
            std::cout << std::endl;
        }
        
        // Verify unit norms
        std::cout << "\nVector Norm Check:" << std::endl;
        int unitNormCount = 0;
        int nonUnitNormCount = 0;
        
        for (const auto& tag : allTags) {
            auto embedding = db.getEmbedding(tag);
            if (!embedding.empty()) {
                float norm = 0.0f;
                for (float val : embedding) {
                    norm += val * val;
                }
                norm = std::sqrt(norm);
                
                if (std::abs(norm - 1.0f) < 0.01f) {
                    unitNormCount++;
                } else {
                    nonUnitNormCount++;
                }
            }
        }
        
        std::cout << "  Unit-normalized vectors: " << unitNormCount << "/" << allTags.size() << std::endl;
        if (nonUnitNormCount > 0) {
            std::cout << "  ⚠ WARNING: " << nonUnitNormCount << " vectors are not unit-normalized" << std::endl;
        } else {
            std::cout << "  ✓ All vectors are unit-normalized" << std::endl;
        }
        
        std::cout << "\n==========================================" << std::endl;
        std::cout << "Database Status: OK" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
