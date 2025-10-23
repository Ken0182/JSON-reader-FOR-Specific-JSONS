/**
 * @file smoke_test.cpp
 * @brief Comprehensive smoke tests for semantic knowledge base
 * @author AI Assistant
 * @version 1.0
 */

#include "../src/semantic_knowledge_base.hpp"
#include "../src/semantic_db.hpp"
#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <filesystem>

class SmokeTester {
private:
    std::unique_ptr<audio_config::SemanticKnowledgeBase> kb_;
    std::string testDbPath_;

public:
    SmokeTester(const std::string& dbPath = "test_smoke.db") : testDbPath_(dbPath) {
        // Clean up any existing test database
        std::filesystem::remove(testDbPath_);
    }

    ~SmokeTester() {
        // Clean up test database
        std::filesystem::remove(testDbPath_);
    }

    bool runAllTests() {
        std::cout << "=== Semantic KB Smoke Tests ===" << std::endl;
        
        bool allPassed = true;
        
        allPassed &= testLoadAndDimension();
        allPassed &= testZeroVectorRepair();
        allPassed &= testLearningWrites();
        allPassed &= testIDFRecompute();
        allPassed &= testEdgeCaseGuards();
        allPassed &= testObservability();
        
        std::cout << "\n=== Test Results ===" << std::endl;
        std::cout << (allPassed ? "✅ ALL TESTS PASSED" : "❌ SOME TESTS FAILED") << std::endl;
        
        return allPassed;
    }

private:
    bool testLoadAndDimension() {
        std::cout << "\n--- Test 1: Load & Dimension ---" << std::endl;
        
        try {
            kb_ = std::make_unique<audio_config::SemanticKnowledgeBase>(testDbPath_);
            if (!kb_->initialize(true)) {
                std::cout << "❌ Failed to initialize KB" << std::endl;
                return false;
            }
            
            // Test basic properties
            int dimension = kb_->getDimension();
            if (dimension <= 0) {
                std::cout << "❌ Invalid dimension: " << dimension << std::endl;
                return false;
            }
            
            std::cout << "✅ KB loaded successfully" << std::endl;
            std::cout << "✅ Dimension: " << dimension << std::endl;
            
            return true;
        } catch (const std::exception& e) {
            std::cout << "❌ Exception: " << e.what() << std::endl;
            return false;
        }
    }

    bool testZeroVectorRepair() {
        std::cout << "\n--- Test 2: Zero Vector Repair ---" << std::endl;
        
        try {
            // First, ensure "warm" exists with a proper embedding
            auto warmEmbedding = kb_->getTagEmbedding("warm");
            if (warmEmbedding.empty()) {
                std::cout << "❌ 'warm' tag not found" << std::endl;
                return false;
            }
            
            // Manually corrupt the embedding in the database
            auto db = kb_->getDatabase();
            if (!db) {
                std::cout << "❌ Cannot access database" << std::endl;
                return false;
            }
            
            // Create a zero vector
            std::vector<float> zeroVector(kb_->getDimension(), 0.0f);
            if (!db->storeEmbedding("warm", zeroVector, "warm")) {
                std::cout << "❌ Failed to store zero vector" << std::endl;
                return false;
            }
            
            std::cout << "✅ Zero vector written to database" << std::endl;
            
            // Now test the repair mechanism
            auto repairedEmbedding = kb_->getTagEmbedding("warm");
            if (repairedEmbedding.empty()) {
                std::cout << "❌ Repair failed - no embedding returned" << std::endl;
                return false;
            }
            
            // Check if it's unit-norm
            float norm = 0.0f;
            for (float val : repairedEmbedding) {
                norm += val * val;
            }
            norm = std::sqrt(norm);
            
            if (std::abs(norm - 1.0f) > 1e-6f) {
                std::cout << "❌ Repaired vector not unit-norm: " << norm << std::endl;
                return false;
            }
            
            std::cout << "✅ Zero vector repaired successfully (norm: " << norm << ")" << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cout << "❌ Exception: " << e.what() << std::endl;
            return false;
        }
    }

    bool testLearningWrites() {
        std::cout << "\n--- Test 3: Learning Writes ---" << std::endl;
        
        try {
            // Test learnTag with custom vector
            std::vector<float> janglyVec(kb_->getDimension(), 0.1f);
            // Make it unit-norm
            float norm = 0.0f;
            for (float val : janglyVec) {
                norm += val * val;
            }
            norm = std::sqrt(norm);
            for (float& val : janglyVec) {
                val /= norm;
            }
            
            if (!kb_->learnTag("jangly", janglyVec, "jangly")) {
                std::cout << "❌ learnTag failed" << std::endl;
                return false;
            }
            
            std::cout << "✅ learnTag('jangly') succeeded" << std::endl;
            
            // Verify the tag exists and is unit-norm
            auto retrieved = kb_->getTagEmbedding("jangly");
            if (retrieved.empty()) {
                std::cout << "❌ 'jangly' not found after learning" << std::endl;
                return false;
            }
            
            float retrievedNorm = 0.0f;
            for (float val : retrieved) {
                retrievedNorm += val * val;
            }
            retrievedNorm = std::sqrt(retrievedNorm);
            
            if (std::abs(retrievedNorm - 1.0f) > 1e-6f) {
                std::cout << "❌ Retrieved vector not unit-norm: " << retrievedNorm << std::endl;
                return false;
            }
            
            std::cout << "✅ Retrieved vector is unit-norm (norm: " << retrievedNorm << ")" << std::endl;
            
            // Test learnTagFromText
            if (!kb_->learnTagFromText("hollow", "hollow airy tone", "hollow")) {
                std::cout << "❌ learnTagFromText failed" << std::endl;
                return false;
            }
            
            std::cout << "✅ learnTagFromText('hollow') succeeded" << std::endl;
            
            // Verify hollow exists and is not zero
            auto hollowEmbedding = kb_->getTagEmbedding("hollow");
            if (hollowEmbedding.empty()) {
                std::cout << "❌ 'hollow' not found after learning" << std::endl;
                return false;
            }
            
            float hollowNorm = 0.0f;
            for (float val : hollowEmbedding) {
                hollowNorm += val * val;
            }
            hollowNorm = std::sqrt(hollowNorm);
            
            if (hollowNorm < 1e-6f) {
                std::cout << "❌ 'hollow' vector is zero" << std::endl;
                return false;
            }
            
            std::cout << "✅ 'hollow' vector is non-zero (norm: " << hollowNorm << ")" << std::endl;
            
            return true;
            
        } catch (const std::exception& e) {
            std::cout << "❌ Exception: " << e.what() << std::endl;
            return false;
        }
    }

    bool testIDFRecompute() {
        std::cout << "\n--- Test 4: IDF Recompute ---" << std::endl;
        
        try {
            // First, ensure the test tags exist in the database
            std::vector<std::string> uniqueTags = {"test_bright", "test_jangly", "test_hollow", "test_warm"};
            for (const auto& tag : uniqueTags) {
                // Create a simple embedding for each tag
                std::vector<float> embedding(kb_->getDimension(), 0.1f);
                // Make it unit-norm
                float norm = 0.0f;
                for (float val : embedding) {
                    norm += val * val;
                }
                norm = std::sqrt(norm);
                for (float& val : embedding) {
                    val /= norm;
                }
                
                if (!kb_->learnTag(tag, embedding, tag)) {
                    std::cout << "❌ Failed to create tag " << tag << std::endl;
                    return false;
                }
            }
            
            // Now update IDF for these tags
            std::vector<std::string> testTags = {
                "test_bright", "test_bright", "test_bright",  // Common tag
                "test_jangly", "test_jangly",                 // Less common
                "test_hollow",                                // Rare
                "test_warm", "test_warm", "test_warm", "test_warm" // Very common
            };
            
            for (const auto& tag : testTags) {
                // Simulate different document frequencies
                int docFreq = (tag == "test_bright" || tag == "test_warm") ? 3 : 
                             (tag == "test_jangly") ? 2 : 1;
                float idf = std::log(4.0f / (1.0f + docFreq)); // 4 total documents
                
                if (!kb_->updateIDF(tag, idf, docFreq)) {
                    std::cout << "❌ Failed to update IDF for " << tag << std::endl;
                    return false;
                }
            }
            
            std::cout << "✅ IDF statistics updated" << std::endl;
            
            // Verify that common tags have lower IDF than rare ones
            auto db = kb_->getDatabase();
            if (!db) {
                std::cout << "❌ Cannot access database" << std::endl;
                return false;
            }
            
            // Check IDF values
            auto brightIDF = db->getIDF("test_bright");
            auto janglyIDF = db->getIDF("test_jangly");
            auto hollowIDF = db->getIDF("test_hollow");
            
            std::cout << "IDF values: test_bright=" << brightIDF << ", test_jangly=" << janglyIDF << ", test_hollow=" << hollowIDF << std::endl;
            
            if (brightIDF < 0 || janglyIDF < 0 || hollowIDF < 0) {
                std::cout << "❌ Some IDF values not found (test_bright=" << brightIDF 
                         << ", test_jangly=" << janglyIDF << ", test_hollow=" << hollowIDF << ")" << std::endl;
                return false;
            }
            
            // Common tags should have lower IDF than rare tags
            if (brightIDF >= janglyIDF || janglyIDF >= hollowIDF) {
                std::cout << "❌ IDF ordering incorrect: test_bright=" << brightIDF 
                         << ", test_jangly=" << janglyIDF << ", test_hollow=" << hollowIDF << std::endl;
                return false;
            }
            
            std::cout << "✅ IDF ordering correct: test_bright=" << brightIDF 
                     << " < test_jangly=" << janglyIDF << " < test_hollow=" << hollowIDF << std::endl;
            
            return true;
            
        } catch (const std::exception& e) {
            std::cout << "❌ Exception: " << e.what() << std::endl;
            return false;
        }
    }

    bool testEdgeCaseGuards() {
        std::cout << "\n--- Test 5: Edge Case Guards ---" << std::endl;
        
        try {
            // Test dimension drift
            std::vector<float> wrongDimVec(kb_->getDimension() + 1, 0.1f);
            
            // This should fail gracefully
            bool result = kb_->learnTag("test_wrong_dim", wrongDimVec, "test_wrong_dim");
            if (result) {
                std::cout << "❌ Should have rejected wrong dimension vector" << std::endl;
                return false;
            }
            
            // The error message should be printed to stderr, which is expected
            
            std::cout << "✅ Dimension drift guard working" << std::endl;
            
            // Test with correct dimension
            std::vector<float> correctDimVec(kb_->getDimension(), 0.1f);
            if (!kb_->learnTag("test_correct_dim", correctDimVec, "test_correct_dim")) {
                std::cout << "❌ Should have accepted correct dimension vector" << std::endl;
                return false;
            }
            
            std::cout << "✅ Correct dimension vector accepted" << std::endl;
            
            return true;
            
        } catch (const std::exception& e) {
            std::cout << "❌ Exception: " << e.what() << std::endl;
            return false;
        }
    }

    bool testObservability() {
        std::cout << "\n--- Test 6: Observability ---" << std::endl;
        
        try {
            // Test that we can get database statistics
            auto db = kb_->getDatabase();
            if (!db) {
                std::cout << "❌ Cannot access database" << std::endl;
                return false;
            }
            
            // Check schema version
            int version = db->getSchemaVersion();
            if (version <= 0) {
                std::cout << "❌ Invalid schema version: " << version << std::endl;
                return false;
            }
            
            std::cout << "✅ Schema version: " << version << std::endl;
            
            // Test embedding functionality
            auto embedding = kb_->getTagEmbedding("jangly");
            if (embedding.empty()) {
                std::cout << "❌ No embedding found for 'jangly'" << std::endl;
                return false;
            }
            
            std::cout << "✅ Embedding found for 'jangly' (dim: " << embedding.size() << ")" << std::endl;
            
            return true;
            
        } catch (const std::exception& e) {
            std::cout << "❌ Exception: " << e.what() << std::endl;
            return false;
        }
    }
};

int main(int argc, char* argv[]) {
    std::string dbPath = "test_smoke.db";
    
    if (argc > 1) {
        dbPath = argv[1];
    }
    
    SmokeTester tester(dbPath);
    bool success = tester.runAllTests();
    
    return success ? 0 : 1;
}