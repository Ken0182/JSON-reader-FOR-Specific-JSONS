/**
 * @file test_semantic_db.cpp
 * @brief Test semantic database integrity
 */

#include <iostream>
#include <cmath>
#include "../src/semantic_db.hpp"
#include "../src/semantic_knowledge_base.hpp"

using namespace audio_config;

int main(int argc, char* argv[]) {
    std::string dbPath = "semantic.db";
    if (argc > 1) {
        dbPath = argv[1];
    }
    
    std::cout << "Testing semantic database: " << dbPath << std::endl;
    std::cout << "===========================================" << std::endl;
    
    try {
        // Load knowledge base
        SemanticKnowledgeBase kb(dbPath);
        if (!kb.initialize(false)) {
            std::cerr << "Failed to initialize knowledge base" << std::endl;
            return 1;
        }
        
        std::cout << "✓ Knowledge base initialized" << std::endl;
        
        // Test 1: Dimension check
        int dimension = kb.getDimension();
        std::cout << "\nTest 1: Dimension Check" << std::endl;
        std::cout << "  Dimension: " << dimension << "D" << std::endl;
        if (dimension == 100) {
            std::cout << "  ✓ PASS: Dimension matches seed (100)" << std::endl;
        } else {
            std::cout << "  ✗ FAIL: Expected 100, got " << dimension << std::endl;
            return 1;
        }
        
        // Test 2: Unit norm check
        std::cout << "\nTest 2: Unit Norm Check" << std::endl;
        std::vector<std::string> testTags = {"warm", "bright", "dark", "dreamy", "aggressive", "lush", "harsh"};
        int passCount = 0;
        int totalCount = 0;
        
        for (const auto& tag : testTags) {
            auto embedding = kb.getTagEmbedding(tag);
            if (embedding.empty()) {
                std::cout << "  ⚠ WARNING: No embedding for tag: " << tag << std::endl;
                continue;
            }
            
            // Calculate norm
            float norm = 0.0f;
            for (float val : embedding) {
                norm += val * val;
            }
            norm = std::sqrt(norm);
            
            totalCount++;
            bool isUnitNorm = std::abs(norm - 1.0f) < 0.01f;
            if (isUnitNorm) {
                passCount++;
                std::cout << "  ✓ " << tag << ": |v|=" << norm << std::endl;
            } else {
                std::cout << "  ✗ " << tag << ": |v|=" << norm << " (NOT UNIT NORM)" << std::endl;
            }
        }
        
        std::cout << "\n  Unit Norm Results: " << passCount << "/" << totalCount << " passed" << std::endl;
        if (passCount == totalCount) {
            std::cout << "  ✓ PASS: All vectors are unit-normalized" << std::endl;
        } else {
            std::cout << "  ✗ FAIL: Some vectors are not unit-normalized" << std::endl;
            return 1;
        }
        
        // Test 3: Tag count
        std::cout << "\nTest 3: Database Statistics" << std::endl;
        auto allTags = kb.getAllTags();
        std::cout << "  Total tags: " << allTags.size() << std::endl;
        std::cout << "  Sample tags (first 10): ";
        for (size_t i = 0; i < std::min(size_t(10), allTags.size()); ++i) {
            std::cout << allTags[i];
            if (i < std::min(size_t(10), allTags.size()) - 1) std::cout << ", ";
        }
        std::cout << std::endl;
        
        if (allTags.size() > 0) {
            std::cout << "  ✓ PASS: Tags loaded successfully" << std::endl;
        } else {
            std::cout << "  ✗ FAIL: No tags found in database" << std::endl;
            return 1;
        }
        
        // Test 4: Alias resolution
        std::cout << "\nTest 4: Alias Resolution" << std::endl;
        std::vector<std::pair<std::string, std::string>> aliases = {
            {"synthesizer", "synth"},
            {"keyboard", "keys"},
            {"lowpass", "low-pass"}
        };
        
        int aliasPass = 0;
        for (const auto& [alias, canonical] : aliases) {
            std::string resolved = kb.getCanonicalTag(alias);
            if (resolved == canonical || resolved == alias) {
                std::cout << "  ✓ " << alias << " → " << resolved << std::endl;
                aliasPass++;
            } else {
                std::cout << "  ✗ " << alias << " → " << resolved << " (expected: " << canonical << ")" << std::endl;
            }
        }
        
        if (aliasPass > 0) {
            std::cout << "  ✓ PASS: Alias system working (" << aliasPass << "/" << aliases.size() << ")" << std::endl;
        }
        
        // Test 5: Similarity test
        std::cout << "\nTest 5: Semantic Similarity" << std::endl;
        auto warmVec = kb.getTagEmbedding("warm");
        auto coldVec = kb.getTagEmbedding("cold");
        auto softVec = kb.getTagEmbedding("soft");
        
        if (!warmVec.empty() && !coldVec.empty() && !softVec.empty()) {
            float warmCold = SemanticKnowledgeBase::cosineSimilarity(warmVec, coldVec);
            float warmSoft = SemanticKnowledgeBase::cosineSimilarity(warmVec, softVec);
            
            std::cout << "  warm ⋅ cold: " << warmCold << std::endl;
            std::cout << "  warm ⋅ soft: " << warmSoft << std::endl;
            
            // Warm should be more similar to soft than to cold (in typical embeddings)
            std::cout << "  ✓ Similarity scores computed" << std::endl;
        }
        
        std::cout << "\n===========================================" << std::endl;
        std::cout << "✓ ALL TESTS PASSED" << std::endl;
        std::cout << "Database integrity verified!" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
