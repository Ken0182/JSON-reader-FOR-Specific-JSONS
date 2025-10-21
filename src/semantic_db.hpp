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
     * @brief Get database file path
     * @return Database file path
     */
    std::string getPath() const { return dbPath_; }

    /**
     * @brief Count total tags (including aliases)
     * @return Number of rows in tags table
     */
    int countTags() const;

    /**
     * @brief Count alias rows (where canonical != tag)
     * @return Number of aliases
     */
    int countAliases() const;

    /**
     * @brief Store an alias mapping without embedding
     * @param aliasTag Alias tag (e.g., "synthesizer")
     * @param canonicalTag Canonical tag (e.g., "synth")
     * @return true if successful
     */
    bool storeAlias(const std::string& aliasTag, const std::string& canonicalTag);
    
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
