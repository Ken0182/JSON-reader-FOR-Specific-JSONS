/**
 * @file semantic_knowledge_base.cpp
 * @brief Semantic Knowledge Base Implementation
 * @author AI Assistant
 * @version 1.6
 */

#include "semantic_knowledge_base.hpp"
#include "text_utils.hpp"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <unordered_map>

namespace audio_config {

SemanticKnowledgeBase::SemanticKnowledgeBase(const std::string& dbPath) {
    // Create database connection
    try {
        db_ = std::make_unique<SemanticDatabase>(dbPath);
    } catch (const std::exception& e) {
        std::cerr << "Failed to open semantic database: " << e.what() << std::endl;
        throw;
    }
}

bool SemanticKnowledgeBase::initialize(bool createDefault) {
    if (!db_ || !db_->isValid()) {
        // Initialize schema
        if (!db_->initializeSchema()) {
            std::cerr << "Failed to initialize database schema" << std::endl;
            return false;
        }
    }
    
    // Get embedding dimension from database
    dimension_ = db_->getEmbeddingDimension();
    bool needsDefaultEmbeddings = false;
    
    if (dimension_ == 0) {
        // No embeddings yet, use default
        dimension_ = 100;
        std::cout << "No embeddings in database, using default dimension: " << dimension_ << std::endl;
        needsDefaultEmbeddings = createDefault;
    } else {
        std::cout << "Loaded semantic database with dimension: " << dimension_ << std::endl;
    }
    
    // CRITICAL: Create sentence encoder BEFORE seeding defaults
    // This ensures encoder is available for createDefaultEmbeddings()
    encoder_ = SentenceEncoder::createDefault(db_.get(), dimension_);
    if (!encoder_ || !encoder_->isReady()) {
        std::cerr << "Failed to create sentence encoder" << std::endl;
        return false;
    }
    
    // Now create default embeddings (if needed) with encoder available
    if (needsDefaultEmbeddings) {
        createDefaultEmbeddings();
    }
    
    isReady_ = true;
    return true;
}

std::vector<float> SemanticKnowledgeBase::encodeText(const std::string& text) const {
    if (!isReady_ || !encoder_) {
        return std::vector<float>(dimension_, 0.0f);
    }
    
    return encoder_->encode(text);
}

std::vector<float> SemanticKnowledgeBase::getTagEmbedding(const std::string& tag) const {
    if (!db_) {
        return {};
    }
    
    // Try direct lookup
    auto embedding = db_->getEmbedding(tag);
    if (!embedding.empty()) {
        return embedding;
    }
    
    // Try canonical form
    std::string canonical = db_->getCanonicalTag(tag);
    if (canonical != tag) {
        embedding = db_->getEmbedding(canonical);
        if (!embedding.empty()) {
            return embedding;
        }
    }
    
    // Not in database, encode using sentence encoder
    if (encoder_) {
        return encoder_->encode(tag);
    }
    
    return {};
}

std::string SemanticKnowledgeBase::getCanonicalTag(const std::string& tag) const {
    if (!db_) return tag;
    return db_->getCanonicalTag(tag);
}

float SemanticKnowledgeBase::getIDF(const std::string& tag) const {
    if (!db_) return 0.0f;
    
    // Try direct lookup
    float idf = db_->getIDF(tag);
    if (idf > 0.0f) {
        return idf;
    }
    
    // Try canonical form
    std::string canonical = db_->getCanonicalTag(tag);
    if (canonical != tag) {
        return db_->getIDF(canonical);
    }
    
    return 0.0f;
}

std::vector<float> SemanticKnowledgeBase::computeConfigEmbedding(
    const std::vector<std::string>& tags) const {
    
    if (tags.empty()) {
        return std::vector<float>(dimension_, 0.0f);
    }
    
    // Accumulate tag embeddings
    std::vector<float> accumulated(dimension_, 0.0f);
    int count = 0;
    
    for (const auto& tag : tags) {
        auto tagEmbed = getTagEmbedding(tag);
        if (!tagEmbed.empty()) {
            // Ensure correct dimension
            if (static_cast<int>(tagEmbed.size()) != dimension_) {
                tagEmbed.resize(dimension_, 0.0f);
            }
            
            for (int i = 0; i < dimension_; ++i) {
                accumulated[i] += tagEmbed[i];
            }
            ++count;
        }
    }
    
    // Average
    if (count > 0) {
        float scale = 1.0f / count;
        for (float& val : accumulated) {
            val *= scale;
        }
    }
    
    // Normalize
    normalizeVector(accumulated);
    return accumulated;
}

float SemanticKnowledgeBase::cosineSimilarity(const std::vector<float>& a, 
                                              const std::vector<float>& b) {
    if (a.size() != b.size() || a.empty()) {
        return 0.0f;
    }
    
    // Dot product (assuming both vectors are normalized)
    float dotProduct = std::inner_product(a.begin(), a.end(), b.begin(), 0.0f);
    
    // Clamp to [0, 1]
    return std::clamp(dotProduct, 0.0f, 1.0f);
}

void SemanticKnowledgeBase::normalizeVector(std::vector<float>& vec) {
    if (vec.empty()) return;
    
    // L2 normalization
    float sumSquares = 0.0f;
    for (float val : vec) {
        sumSquares += val * val;
    }
    
    if (sumSquares > 1e-8f) {
        float norm = std::sqrt(sumSquares);
        for (float& val : vec) {
            val /= norm;
        }
    }
}

std::vector<std::string> SemanticKnowledgeBase::getAllTags() const {
    if (!db_) return {};
    return db_->getAllTags();
}

float SemanticKnowledgeBase::getConfigParam(const std::string& key, float defaultValue) const {
    if (!db_) return defaultValue;
    return db_->getConfigValue(key, defaultValue);
}

bool SemanticKnowledgeBase::storeTagEmbedding(const std::string& tag,
                                              const std::vector<float>& embedding,
                                              const std::string& canonical) {
    if (!db_) return false;
    return db_->storeEmbedding(tag, embedding, canonical);
}

bool SemanticKnowledgeBase::storeIDF(const std::string& tag, float idf, int docCount) {
    if (!db_) return false;
    return db_->storeIDF(tag, idf, docCount);
}

bool SemanticKnowledgeBase::storeConfig(const std::string& key, float value) {
    if (!db_) return false;
    return db_->storeConfig(key, value);
}

int SemanticKnowledgeBase::computeIDFStatistics(const std::vector<std::vector<std::string>>& docs) {
    if (!db_ || docs.empty()) return 0;
    
    // Count document frequency (number of documents containing each tag)
    std::unordered_map<std::string, int> docFrequency;
    
    for (const auto& docTags : docs) {
        // Get unique canonical tags in this document
        std::unordered_set<std::string> uniqueTagsInDoc;
        for (const auto& tag : docTags) {
            std::string canonical = getCanonicalTag(tag);
            uniqueTagsInDoc.insert(canonical);
        }
        
        // Increment document frequency for each unique tag
        for (const auto& canonicalTag : uniqueTagsInDoc) {
            docFrequency[canonicalTag]++;
        }
    }
    
    // Compute IDF for each tag
    int totalDocs = static_cast<int>(docs.size());
    int storedCount = 0;
    
    for (const auto& [tag, docCount] : docFrequency) {
        // IDF = log(totalDocs / docFreq)
        // Higher IDF means more discriminative (appears in fewer documents)
        float idf = std::log(static_cast<float>(totalDocs) / static_cast<float>(docCount));
        
        // Store both IDF and the actual document count
        if (db_->storeIDF(tag, idf, docCount)) {
            ++storedCount;
        }
    }
    
    std::cout << "Computed IDF for " << storedCount << " unique tags across " 
              << totalDocs << " documents" << std::endl;
    return storedCount;
}

bool SemanticKnowledgeBase::storeUserSignal(const std::string& token, float strength, int64_t lastUpdate) {
    if (!db_) return false;
    return db_->storeUserSignal(token, strength, lastUpdate);
}

std::unordered_map<std::string, std::pair<float, int64_t>> SemanticKnowledgeBase::loadUserSignals() const {
    if (!db_) return {};
    return db_->loadUserSignals();
}

bool SemanticKnowledgeBase::storeQueryHistory(const std::string& queryText, const std::string& tokens,
                                             int64_t timestamp, float rawStrength) {
    if (!db_) return false;
    return db_->storeQueryHistory(queryText, tokens, timestamp, rawStrength);
}

std::vector<std::tuple<std::string, std::string, int64_t, float>>
SemanticKnowledgeBase::loadQueryHistory(int maxResults) const {
    if (!db_) return {};
    return db_->loadQueryHistory(maxResults);
}

bool SemanticKnowledgeBase::clearUserSignals() {
    if (!db_) return false;
    return db_->clearUserSignals();
}

bool SemanticKnowledgeBase::clearQueryHistory() {
    if (!db_) return false;
    return db_->clearQueryHistory();
}

void SemanticKnowledgeBase::createDefaultEmbeddings() {
    if (!db_) return;
    
    // CRITICAL: Ensure encoder is ready before seeding
    if (!encoder_ || !encoder_->isReady()) {
        std::cerr << "Warning: Cannot create default embeddings - encoder not ready" << std::endl;
        return;
    }
    
    std::cout << "Creating default embeddings..." << std::endl;
    
    // Create default embeddings for common audio descriptor tags
    // These are manually curated semantic vectors
    
    struct TagData {
        std::string tag;
        std::string canonical;
        std::vector<std::string> semanticNeighbors;
    };
    
    std::vector<TagData> commonTags = {
        {"warm", "warm", {"soft", "smooth", "mellow", "round"}},
        {"bright", "bright", {"crisp", "clear", "sharp", "brilliant"}},
        {"dark", "dark", {"deep", "heavy", "thick", "shadowy"}},
        {"analog", "analog", {"vintage", "warm", "classic", "retro"}},
        {"digital", "digital", {"clean", "precise", "modern", "crisp"}},
        {"fat", "fat", {"thick", "heavy", "full", "rich"}},
        {"thin", "thin", {"light", "airy", "sparse", "minimal"}},
        {"punchy", "punchy", {"sharp", "attack", "percussive", "dynamic"}},
        {"smooth", "smooth", {"soft", "flowing", "gentle", "silky"}},
        {"aggressive", "aggressive", {"harsh", "intense", "powerful", "bold"}},
        {"mellow", "mellow", {"smooth", "calm", "gentle", "soft"}},
        {"lush", "lush", {"rich", "full", "dense", "layered"}},
        {"sparse", "sparse", {"minimal", "thin", "simple", "clean"}},
        {"vintage", "vintage", {"retro", "classic", "old", "analog"}},
        {"modern", "modern", {"new", "contemporary", "fresh", "digital"}},
        {"organic", "organic", {"natural", "acoustic", "real", "warm"}},
        {"synthetic", "synthetic", {"artificial", "digital", "processed", "electronic"}},
        {"dreamy", "dreamy", {"ethereal", "floating", "ambient", "soft"}},
        {"metallic", "metallic", {"bright", "sharp", "cold", "digital"}},
        {"wooden", "wooden", {"acoustic", "natural", "warm", "organic"}}
    };
    
    // Generate simple semantic embeddings
    // For production, these would be from a pre-trained model
    int storedCount = 0;
    int zeroVectorCount = 0;
    
    for (const auto& tagData : commonTags) {
        // Generate hash-based embedding using sentence encoder
        std::vector<float> embedding = encoder_->encode(tagData.tag);
        
        // Verify embedding is non-zero
        float norm = 0.0f;
        for (float val : embedding) {
            norm += val * val;
        }
        norm = std::sqrt(norm);
        
        if (norm < 1e-6f) {
            std::cerr << "Warning: Zero-length embedding generated for tag: " << tagData.tag << std::endl;
            ++zeroVectorCount;
            continue;  // Skip storing zero vectors
        }
        
        if (db_->storeEmbedding(tagData.tag, embedding, tagData.canonical)) {
            ++storedCount;
        }
    }
    
    std::cout << "Created " << storedCount << " default embeddings";
    if (zeroVectorCount > 0) {
        std::cout << " (skipped " << zeroVectorCount << " zero vectors)";
    }
    std::cout << std::endl;
    
    // Verify: Read back one tag to ensure persistence worked
    auto verifyEmbed = db_->getEmbedding("warm");
    if (verifyEmbed.empty()) {
        std::cerr << "Warning: Could not verify stored embeddings" << std::endl;
    } else {
        float verifyNorm = 0.0f;
        for (float val : verifyEmbed) {
            verifyNorm += val * val;
        }
        verifyNorm = std::sqrt(verifyNorm);
        if (verifyNorm < 1e-6f) {
            std::cerr << "Warning: Stored embedding has zero length - encoder may not be working correctly" << std::endl;
        }
    }
}

} // namespace audio_config
