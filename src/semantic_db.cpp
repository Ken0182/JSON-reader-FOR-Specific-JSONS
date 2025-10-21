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
    
    // Create indices for performance
    executeSql("CREATE INDEX IF NOT EXISTS idx_canonical ON tags(canonical);");
    executeSql("CREATE INDEX IF NOT EXISTS idx_idf ON idf_stats(idf DESC);");
    
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
        if (blobSize == dimension * sizeof(float) && blobData) {
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

int SemanticDatabase::countTags() const {
    if (!db_) return 0;
    sqlite3_stmt* stmt = nullptr;
    int count = 0;
    const char* sql = "SELECT COUNT(*) FROM tags;";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    return count;
}

int SemanticDatabase::countAliases() const {
    if (!db_) return 0;
    sqlite3_stmt* stmt = nullptr;
    int count = 0;
    const char* sql = "SELECT COUNT(*) FROM tags WHERE canonical <> tag;";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    return count;
}

bool SemanticDatabase::storeAlias(const std::string& aliasTag, const std::string& canonicalTag) {
    if (!db_ || aliasTag.empty() || canonicalTag.empty()) return false;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT OR REPLACE INTO tags (tag, canonical, dimension) VALUES (?, ?, COALESCE((SELECT dimension FROM tags WHERE tag = ?), 100));";
    bool success = false;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, aliasTag.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, canonicalTag.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, canonicalTag.c_str(), -1, SQLITE_TRANSIENT);
        success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
    }
    return success;
}

} // namespace audio_config
