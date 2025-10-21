/**
 * @file semantic_knowledge_base.hpp
 * @brief Semantic Knowledge Base - High-Level Interface
 * @author AI Assistant
 * @version 1.6
 * 
 * v1.6 Semantic Knowledge Base:
 * - Unified interface for semantic operations
 * - Combines database lookups with sentence encoding
 * - Precomputes and caches entry embeddings
 * - Supports unlimited vocabulary via sentence encoder
 * - Handles tag canonicalization and IDF scoring
 * 
 * ARCHITECTURE:
 * 
 * SemanticKnowledgeBase
 *   ├─ SemanticDatabase (SQLite storage)
 *   │   ├─ Tag embeddings (curated high-quality)
 *   │   ├─ Tag aliases (synonyms)
 *   │   ├─ IDF statistics
 *   │   └─ Config parameters
 *   └─ SentenceEncoder (text → vector)
 *       ├─ Database lookup (known tags)
 *       └─ Hash-based fallback (unknown words)
 * 
 * USAGE:
 *   SemanticKnowledgeBase kb("semantic.db");
 *   auto queryVec = kb.encodeQuery("dreamy but not lush");
 *   auto configVec = kb.getConfigEmbedding(config);
 *   float score = kb.similarity(queryVec, configVec);
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "semantic_db.hpp"
#include "sentence_encoder.hpp"

namespace audio_config {

// Forward declarations
class AudioConfig;

/**
 * @brief Semantic Knowledge Base
 * 
 * High-level interface combining database and sentence encoder.
 * Provides semantic operations for audio configuration system.
 */
class SemanticKnowledgeBase {
public:
    /**
     * @brief Constructor
     * @param dbPath Path to SQLite semantic database
     * @throws std::runtime_error if database cannot be loaded
     */
    explicit SemanticKnowledgeBase(const std::string& dbPath);
    
    /**
     * @brief Destructor
     */
    ~SemanticKnowledgeBase() = default;
    
    /**
     * @brief Initialize knowledge base
     * @param createDefault If true, create default schema/data
     * @return true if successful
     */
    bool initialize(bool createDefault = false);
    
    /**
     * @brief Encode arbitrary text into semantic vector
     * @param text Input text (query, description)
     * @return Normalized embedding vector
     * 
     * Uses database lookup for known tags, sentence encoder for unknown words
     */
    std::vector<float> encodeText(const std::string& text) const;
    
    /**
     * @brief Get embedding for a single tag
     * @param tag Tag to look up
     * @return Embedding vector (empty if not found)
     */
    std::vector<float> getTagEmbedding(const std::string& tag) const;
    
    /**
     * @brief Get canonical form of tag (resolves aliases)
     * @param tag Input tag
     * @return Canonical tag
     */
    std::string getCanonicalTag(const std::string& tag) const;
    
    /**
     * @brief Get IDF score for tag
     * @param tag Tag to look up
     * @return IDF score
     */
    float getIDF(const std::string& tag) const;
    
    /**
     * @brief Compute configuration embedding from tags
     * @param tags Tag list
     * @return Normalized average embedding
     */
    std::vector<float> computeConfigEmbedding(const std::vector<std::string>& tags) const;
    
    /**
     * @brief Calculate cosine similarity between vectors
     * @param a First vector (should be normalized)
     * @param b Second vector (should be normalized)
     * @return Similarity score [0, 1]
     */
    static float cosineSimilarity(const std::vector<float>& a, 
                                  const std::vector<float>& b);
    
    /**
     * @brief Normalize vector to unit length
     * @param vec Vector to normalize (modified in place)
     */
    static void normalizeVector(std::vector<float>& vec);
    
    /**
     * @brief Get embedding dimension
     * @return Vector dimension
     */
    int getDimension() const { return dimension_; }
    
    /**
     * @brief Check if knowledge base is ready
     * @return true if initialized and ready
     */
    bool isReady() const { return isReady_; }
    
    /**
     * @brief Get all tags in database
     * @return Vector of tag names
     */
    std::vector<std::string> getAllTags() const;
    
    /**
     * @brief Get configuration parameter
     * @param key Parameter name
     * @param defaultValue Default if not found
     * @return Parameter value
     */
    float getConfigParam(const std::string& key, float defaultValue = 0.0f) const;
    
    /**
     * @brief Store tag embedding in database
     * @param tag Tag name
     * @param embedding Embedding vector
     * @param canonical Canonical form (optional)
     * @return true if successful
     */
    bool storeTagEmbedding(const std::string& tag,
                          const std::vector<float>& embedding,
                          const std::string& canonical = "");
    
    /**
     * @brief Store IDF statistic
     * @param tag Tag name
     * @param idf IDF score
     * @param docCount Document count
     * @return true if successful
     */
    bool storeIDF(const std::string& tag, float idf, int docCount);
    
    /**
     * @brief Store configuration parameter
     * @param key Parameter name
     * @param value Parameter value
     * @return true if successful
     */
    bool storeConfig(const std::string& key, float value);
    
    /**
     * @brief Compute IDF statistics from configuration corpus
     * @param allTags All tags from all configurations
     * @return Number of tags processed
     */
    int computeIDFStatistics(const std::vector<std::string>& allTags);
    
private:
    std::unique_ptr<SemanticDatabase> db_;
    std::unique_ptr<SentenceEncoder> encoder_;
    int dimension_{100};
    bool isReady_{false};
    
    /**
     * @brief Create default embeddings for common audio tags
     */
    void createDefaultEmbeddings();
};

} // namespace audio_config
