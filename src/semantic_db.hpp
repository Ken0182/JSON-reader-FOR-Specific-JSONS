/**
 * @file semantic_db.hpp
 * @brief Semantic Knowledge Base - SQLite Database Interface
 * @author AI Assistant
 * @version 1.6
 * 
 * v1.6 Semantic Knowledge Base:
 * - SQLite database for tags, embeddings, aliases, IDF stats
 * - Dynamic embedding dimensions (no hardcoded sizes)
 * - Updateable knowledge via database files (no code changes)
 * - Canonical tag mapping (aliases → canonical forms)
 * - IDF statistics for information-weighted scoring
 * - Configurable tuning parameters stored in DB
 * 
 * DATABASE SCHEMA:
 * 
 * tags (tag TEXT PRIMARY KEY, canonical TEXT, dimension INTEGER)
 *   - tag: unique tag identifier
 *   - canonical: canonical form (e.g., "synth" for "synthesizer")
 *   - dimension: embedding vector dimension
 * 
 * embeddings (tag TEXT PRIMARY KEY, embedding BLOB, dimension INTEGER)
 *   - tag: tag identifier (links to tags table)
 *   - embedding: binary float array (little-endian)
 *   - dimension: vector dimension (for validation)
 * 
 * idf_stats (tag TEXT PRIMARY KEY, idf REAL, doc_count INTEGER)
 *   - tag: tag identifier
 *   - idf: inverse document frequency score
 *   - doc_count: number of documents containing this tag
 * 
 * config (key TEXT PRIMARY KEY, value REAL)
 *   - key: parameter name (e.g., "semantic_weight", "alpha")
 *   - value: parameter value
 * 
 * USAGE:
 *   SemanticDatabase db("path/to/semantic.db");
 *   auto embedding = db.getEmbedding("warm");
 *   auto canonical = db.getCanonicalTag("synthesizer");  // → "synth"
 *   float idf = db.getIDF("analog");
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>
#include <sqlite3.h>

namespace audio_config {

/**
 * @brief Semantic Knowledge Database
 * 
 * Manages SQLite database containing:
 * - Tag embeddings (dynamic dimensions)
 * - Tag aliases (synonyms → canonical forms)
 * - IDF statistics (tag informativeness)
 * - Tuning parameters (weights, thresholds)
 */
class SemanticDatabase {
public:
    /**
     * @brief Constructor - opens/creates database
     * @param dbPath Path to SQLite database file
     * @throws std::runtime_error if database cannot be opened
     */
    explicit SemanticDatabase(const std::string& dbPath);
    
    /**
     * @brief Destructor - closes database connection
     */
    ~SemanticDatabase();
    
    // Delete copy/move (manage SQLite connection carefully)
    SemanticDatabase(const SemanticDatabase&) = delete;
    SemanticDatabase& operator=(const SemanticDatabase&) = delete;
    SemanticDatabase(SemanticDatabase&&) = delete;
    SemanticDatabase& operator=(SemanticDatabase&&) = delete;
    
    /**
     * @brief Initialize database schema
     * Creates tables if they don't exist
     * @return true if successful
     */
    bool initializeSchema();
    
    /**
     * @brief Get embedding vector for a tag
     * @param tag Tag to look up
     * @return Embedding vector (empty if not found)
     */
    std::vector<float> getEmbedding(const std::string& tag) const;
    
    /**
     * @brief Get canonical form of a tag (resolves aliases)
     * @param tag Input tag (possibly an alias)
     * @return Canonical tag (or original if no mapping exists)
     */
    std::string getCanonicalTag(const std::string& tag) const;
    
    /**
     * @brief Get IDF score for a tag
     * @param tag Tag to look up
     * @return IDF score (0.0 if not found)
     */
    float getIDF(const std::string& tag) const;
    
    /**
     * @brief Get configuration parameter
     * @param key Parameter name
     * @param defaultValue Default value if not found
     * @return Parameter value
     */
    float getConfigValue(const std::string& key, float defaultValue = 0.0f) const;
    
    /**
     * @brief Get all tags in database
     * @return Vector of all tag names
     */
    std::vector<std::string> getAllTags() const;
    
    /**
     * @brief Get embedding dimension (assumes all embeddings same dimension)
     * @return Embedding dimension (0 if no embeddings)
     */
    int getEmbeddingDimension() const;
    
    /**
     * @brief Check if database is valid and ready
     * @return true if database is open and schema exists
     */
    bool isValid() const;
    
    /**
     * @brief Store embedding for a tag
     * @param tag Tag name
     * @param embedding Embedding vector
     * @param canonical Canonical form (optional, defaults to tag)
     * @return true if successful
     */
    bool storeEmbedding(const std::string& tag, 
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
     * @brief Store user signal (for search interest tracking)
     * @param token Token name
     * @param strength Signal strength
     * @param lastUpdate Timestamp of last update (Unix time)
     * @return true if successful
     */
    bool storeUserSignal(const std::string& token, float strength, int64_t lastUpdate);
    
    /**
     * @brief Load all user signals
     * @return Map of token → (strength, lastUpdate)
     */
    std::unordered_map<std::string, std::pair<float, int64_t>> loadUserSignals() const;
    
    /**
     * @brief Store query history record
     * @param queryText Original query text
     * @param tokens Tokenized query
     * @param timestamp Query timestamp (Unix time)
     * @param rawStrength Initial signal strength
     * @return true if successful
     */
    bool storeQueryHistory(const std::string& queryText, const std::string& tokens,
                          int64_t timestamp, float rawStrength);
    
    /**
     * @brief Load query history (most recent first)
     * @param maxResults Maximum number of records to return
     * @return Vector of (queryText, tokens, timestamp, rawStrength)
     */
    std::vector<std::tuple<std::string, std::string, int64_t, float>> 
        loadQueryHistory(int maxResults = 100) const;
    
    /**
     * @brief Clear all user signals
     * @return true if successful
     */
    bool clearUserSignals();
    
    /**
     * @brief Clear query history
     * @return true if successful
     */
    bool clearQueryHistory();
    
    /**
     * @brief Get database file path
     * @return Database file path
     */
    std::string getPath() const { return dbPath_; }
    
private:
    sqlite3* db_{nullptr};
    std::string dbPath_;
    mutable int cachedDimension_{0};
    
    // Cached prepared statements for performance
    mutable sqlite3_stmt* stmtGetEmbedding_{nullptr};
    mutable sqlite3_stmt* stmtGetCanonical_{nullptr};
    mutable sqlite3_stmt* stmtGetIDF_{nullptr};
    mutable sqlite3_stmt* stmtGetConfig_{nullptr};
    
    /**
     * @brief Prepare frequently-used statements
     */
    void prepareStatements();
    
    /**
     * @brief Finalize prepared statements
     */
    void finalizeStatements();
    
    /**
     * @brief Execute SQL statement (helper)
     * @param sql SQL statement
     * @return true if successful
     */
    bool executeSql(const std::string& sql) const;
};

} // namespace audio_config
