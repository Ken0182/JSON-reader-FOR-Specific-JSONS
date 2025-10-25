/**
 * @file semantic_db.cpp
 * @brief Semantic Knowledge Base - SQLite Implementation
 * @author AI Assistant
 * @version 1.6
 */

#include "semantic_db.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <stdexcept>

namespace audio_config {

SemanticDatabase::SemanticDatabase(const std::string& dbPath) 
    : dbPath_(dbPath) {
    
    // Open database (create if doesn't exist)
    int rc = sqlite3_open(dbPath.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::string error = sqlite3_errmsg(db_);
        sqlite3_close(db_);
        db_ = nullptr;
        throw std::runtime_error("Cannot open semantic database: " + error);
    }
    
    // Enable foreign keys
    executeSql("PRAGMA foreign_keys = ON;");
    
    // Prepare frequently-used statements
    prepareStatements();
}

SemanticDatabase::~SemanticDatabase() {
    finalizeStatements();
    if (db_) {
        sqlite3_close(db_);
    }
}

bool SemanticDatabase::initializeSchema() {
    if (!db_) return false;
    
    // Tags table
    if (!executeSql(R"(
        CREATE TABLE IF NOT EXISTS tags (
            tag TEXT PRIMARY KEY,
            canonical TEXT NOT NULL,
            dimension INTEGER NOT NULL DEFAULT 100
        );
    )")) return false;
    
    // Embeddings table
    if (!executeSql(R"(
        CREATE TABLE IF NOT EXISTS embeddings (
            tag TEXT PRIMARY KEY,
            embedding BLOB NOT NULL,
            dimension INTEGER NOT NULL,
            FOREIGN KEY (tag) REFERENCES tags(tag)
        );
    )")) return false;
    
    // IDF statistics table
    if (!executeSql(R"(
        CREATE TABLE IF NOT EXISTS idf_stats (
            tag TEXT PRIMARY KEY,
            idf REAL NOT NULL,
            doc_count INTEGER NOT NULL DEFAULT 0
        );
    )")) return false;
    
    // Configuration parameters table
    if (!executeSql(R"(
        CREATE TABLE IF NOT EXISTS config (
            key TEXT PRIMARY KEY,
            value REAL NOT NULL
        );
    )")) return false;
    
    // User signals table (for search interest tracking)
    if (!executeSql(R"(
        CREATE TABLE IF NOT EXISTS user_signals (
            token TEXT PRIMARY KEY,
            strength REAL NOT NULL,
            last_update INTEGER NOT NULL
        );
    )")) return false;
    
    // Query history table (for learning patterns)
    if (!executeSql(R"(
        CREATE TABLE IF NOT EXISTS query_history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            query_text TEXT NOT NULL,
            tokens TEXT NOT NULL,
            timestamp INTEGER NOT NULL,
            raw_strength REAL NOT NULL DEFAULT 1.0
        );
    )")) return false;
    
    // Create indices for performance
    executeSql("CREATE INDEX IF NOT EXISTS idx_canonical ON tags(canonical);");
    executeSql("CREATE INDEX IF NOT EXISTS idx_idf ON idf_stats(idf DESC);");
    executeSql("CREATE INDEX IF NOT EXISTS idx_signal_strength ON user_signals(strength DESC);");
    executeSql("CREATE INDEX IF NOT EXISTS idx_query_timestamp ON query_history(timestamp DESC);");
    
    return true;
}

void SemanticDatabase::prepareStatements() {
    if (!db_) return;
    
    // Prepare getEmbedding statement
    const char* sqlEmbed = "SELECT embedding, dimension FROM embeddings WHERE tag = ?;";
    sqlite3_prepare_v2(db_, sqlEmbed, -1, &stmtGetEmbedding_, nullptr);
    
    // Prepare getCanonicalTag statement
    const char* sqlCanon = "SELECT canonical FROM tags WHERE tag = ?;";
    sqlite3_prepare_v2(db_, sqlCanon, -1, &stmtGetCanonical_, nullptr);
    
    // Prepare getIDF statement
    const char* sqlIDF = "SELECT idf FROM idf_stats WHERE tag = ?;";
    sqlite3_prepare_v2(db_, sqlIDF, -1, &stmtGetIDF_, nullptr);
    
    // Prepare getConfig statement
    const char* sqlConfig = "SELECT value FROM config WHERE key = ?;";
    sqlite3_prepare_v2(db_, sqlConfig, -1, &stmtGetConfig_, nullptr);
}

void SemanticDatabase::finalizeStatements() {
    if (stmtGetEmbedding_) sqlite3_finalize(stmtGetEmbedding_);
    if (stmtGetCanonical_) sqlite3_finalize(stmtGetCanonical_);
    if (stmtGetIDF_) sqlite3_finalize(stmtGetIDF_);
    if (stmtGetConfig_) sqlite3_finalize(stmtGetConfig_);
}

std::vector<float> SemanticDatabase::getEmbedding(const std::string& tag) const {
    if (!stmtGetEmbedding_) return {};
    
    // Reset statement
    sqlite3_reset(stmtGetEmbedding_);
    sqlite3_bind_text(stmtGetEmbedding_, 1, tag.c_str(), -1, SQLITE_TRANSIENT);
    
    std::vector<float> embedding;
    
    if (sqlite3_step(stmtGetEmbedding_) == SQLITE_ROW) {
        // Get blob data
        const void* blobData = sqlite3_column_blob(stmtGetEmbedding_, 0);
        int blobSize = sqlite3_column_bytes(stmtGetEmbedding_, 0);
        int dimension = sqlite3_column_int(stmtGetEmbedding_, 1);
        
        // Validate size
       if (blobSize == static_cast<int>(dimension * sizeof(float)) && blobData) {
            embedding.resize(dimension);
            std::memcpy(embedding.data(), blobData, blobSize);
        }
    }
    
    return embedding;
}

std::string SemanticDatabase::getCanonicalTag(const std::string& tag) const {
    if (!stmtGetCanonical_) return tag;
    
    // Reset statement
    sqlite3_reset(stmtGetCanonical_);
    sqlite3_bind_text(stmtGetCanonical_, 1, tag.c_str(), -1, SQLITE_TRANSIENT);
    
    if (sqlite3_step(stmtGetCanonical_) == SQLITE_ROW) {
        const char* canonical = reinterpret_cast<const char*>(
            sqlite3_column_text(stmtGetCanonical_, 0));
        if (canonical) {
            return std::string(canonical);
        }
    }
    
    // No mapping found, return original tag
    return tag;
}

float SemanticDatabase::getIDF(const std::string& tag) const {
    if (!stmtGetIDF_) return 0.0f;
    
    // Reset statement
    sqlite3_reset(stmtGetIDF_);
    sqlite3_bind_text(stmtGetIDF_, 1, tag.c_str(), -1, SQLITE_TRANSIENT);
    
    if (sqlite3_step(stmtGetIDF_) == SQLITE_ROW) {
        return static_cast<float>(sqlite3_column_double(stmtGetIDF_, 0));
    }
    
    return 0.0f;
}

float SemanticDatabase::getConfigValue(const std::string& key, float defaultValue) const {
    if (!stmtGetConfig_) return defaultValue;
    
    // Reset statement
    sqlite3_reset(stmtGetConfig_);
    sqlite3_bind_text(stmtGetConfig_, 1, key.c_str(), -1, SQLITE_TRANSIENT);
    
    if (sqlite3_step(stmtGetConfig_) == SQLITE_ROW) {
        return static_cast<float>(sqlite3_column_double(stmtGetConfig_, 0));
    }
    
    return defaultValue;
}

std::vector<std::string> SemanticDatabase::getAllTags() const {
    if (!db_) return {};
    
    std::vector<std::string> tags;
    sqlite3_stmt* stmt = nullptr;
    
    const char* sql = "SELECT tag FROM tags ORDER BY tag;";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* tag = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            if (tag) {
                tags.emplace_back(tag);
            }
        }
        sqlite3_finalize(stmt);
    }
    
    return tags;
}

int SemanticDatabase::getEmbeddingDimension() const {
    if (cachedDimension_ > 0) return cachedDimension_;
    if (!db_) return 0;
    
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT dimension FROM embeddings LIMIT 1;";
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            cachedDimension_ = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    
    return cachedDimension_;
}

bool SemanticDatabase::isValid() const {
    if (!db_) return false;
    
    // Check if tables exist
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT name FROM sqlite_master WHERE type='table' AND name='tags';";
    
    bool valid = false;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        valid = (sqlite3_step(stmt) == SQLITE_ROW);
        sqlite3_finalize(stmt);
    }
    
    return valid;
}

bool SemanticDatabase::storeEmbedding(const std::string& tag, 
                                     const std::vector<float>& embedding,
                                     const std::string& canonical) {
    if (!db_ || embedding.empty()) return false;
    
    // Begin transaction
    executeSql("BEGIN TRANSACTION;");
    
    // Insert/update tags table
    std::string canonicalTag = canonical.empty() ? tag : canonical;
    sqlite3_stmt* stmtTag = nullptr;
    const char* sqlTag = "INSERT OR REPLACE INTO tags (tag, canonical, dimension) VALUES (?, ?, ?);";
    
    if (sqlite3_prepare_v2(db_, sqlTag, -1, &stmtTag, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmtTag, 1, tag.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmtTag, 2, canonicalTag.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmtTag, 3, static_cast<int>(embedding.size()));
        sqlite3_step(stmtTag);
        sqlite3_finalize(stmtTag);
    }
    
    // Insert/update embeddings table
    sqlite3_stmt* stmtEmbed = nullptr;
    const char* sqlEmbed = "INSERT OR REPLACE INTO embeddings (tag, embedding, dimension) VALUES (?, ?, ?);";
    
    bool success = false;
    if (sqlite3_prepare_v2(db_, sqlEmbed, -1, &stmtEmbed, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmtEmbed, 1, tag.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_blob(stmtEmbed, 2, embedding.data(), 
                         static_cast<int>(embedding.size() * sizeof(float)), 
                         SQLITE_TRANSIENT);
        sqlite3_bind_int(stmtEmbed, 3, static_cast<int>(embedding.size()));
        success = (sqlite3_step(stmtEmbed) == SQLITE_DONE);
        sqlite3_finalize(stmtEmbed);
    }
    
    // Commit transaction
    executeSql(success ? "COMMIT;" : "ROLLBACK;");
    
    return success;
}

bool SemanticDatabase::storeIDF(const std::string& tag, float idf, int docCount) {
    if (!db_) return false;
    
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT OR REPLACE INTO idf_stats (tag, idf, doc_count) VALUES (?, ?, ?);";
    
    bool success = false;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, tag.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 2, idf);
        sqlite3_bind_int(stmt, 3, docCount);
        success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
    }
    
    return success;
}

bool SemanticDatabase::storeConfig(const std::string& key, float value) {
    if (!db_) return false;
    
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT OR REPLACE INTO config (key, value) VALUES (?, ?);";
    
    bool success = false;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 2, value);
        success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
    }
    
    return success;
}

bool SemanticDatabase::storeUserSignal(const std::string& token, float strength, int64_t lastUpdate) {
    if (!db_) return false;
    
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT OR REPLACE INTO user_signals (token, strength, last_update) VALUES (?, ?, ?);";
    
    bool success = false;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 2, strength);
        sqlite3_bind_int64(stmt, 3, lastUpdate);
        success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
    }
    
    return success;
}

std::unordered_map<std::string, std::pair<float, int64_t>> SemanticDatabase::loadUserSignals() const {
    std::unordered_map<std::string, std::pair<float, int64_t>> signals;
    if (!db_) return signals;
    
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT token, strength, last_update FROM user_signals;";
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* token = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            float strength = static_cast<float>(sqlite3_column_double(stmt, 1));
            int64_t lastUpdate = sqlite3_column_int64(stmt, 2);
            
            if (token) {
                signals[std::string(token)] = {strength, lastUpdate};
            }
        }
        sqlite3_finalize(stmt);
    }
    
    return signals;
}

bool SemanticDatabase::storeQueryHistory(const std::string& queryText, const std::string& tokens,
                                        int64_t timestamp, float rawStrength) {
    if (!db_) return false;
    
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT INTO query_history (query_text, tokens, timestamp, raw_strength) VALUES (?, ?, ?, ?);";
    
    bool success = false;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, queryText.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, tokens.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 3, timestamp);
        sqlite3_bind_double(stmt, 4, rawStrength);
        success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
    }
    
    return success;
}

std::vector<std::tuple<std::string, std::string, int64_t, float>> 
SemanticDatabase::loadQueryHistory(int maxResults) const {
    std::vector<std::tuple<std::string, std::string, int64_t, float>> history;
    if (!db_) return history;
    
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT query_text, tokens, timestamp, raw_strength FROM query_history "
                     "ORDER BY timestamp DESC LIMIT ?;";
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, maxResults);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* queryText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            const char* tokens = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            int64_t timestamp = sqlite3_column_int64(stmt, 2);
            float rawStrength = static_cast<float>(sqlite3_column_double(stmt, 3));
            
            if (queryText && tokens) {
                history.emplace_back(std::string(queryText), std::string(tokens), 
                                    timestamp, rawStrength);
            }
        }
        sqlite3_finalize(stmt);
    }
    
    return history;
}

bool SemanticDatabase::clearUserSignals() {
    return executeSql("DELETE FROM user_signals;");
}

bool SemanticDatabase::clearQueryHistory() {
    return executeSql("DELETE FROM query_history;");
}

bool SemanticDatabase::executeSql(const std::string& sql) const {
    if (!db_) return false;
    
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    
    if (rc != SQLITE_OK) {
        if (errMsg) {
            std::cerr << "SQL error: " << errMsg << std::endl;
            sqlite3_free(errMsg);
        }
        return false;
    }
    
    return true;
}

} // namespace audio_config
