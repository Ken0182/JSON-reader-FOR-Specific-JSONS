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
    
    // First, create sentence encoder so default seeding can encode properly
    // Probe dimension from DB; if none, use provisional default for encoder
    dimension_ = db_->getEmbeddingDimension();
    if (dimension_ <= 0) {
        dimension_ = 100;
        std::cout << "No embeddings in database, provisional dimension: " << dimension_ << std::endl;
    } else {
        std::cout << "Loaded semantic database with dimension: " << dimension_ << std::endl;
    }
    
    // Create sentence encoder before any default embedding creation
    encoder_ = SentenceEncoder::createDefault(db_.get(), dimension_);
    if (!encoder_ || !encoder_->isReady()) {
        std::cerr << "Failed to create sentence encoder" << std::endl;
        return false;
    }
    
    // After encoder is ready, if no embeddings exist and createDefault requested, seed defaults
    if (createDefault && db_->getEmbeddingDimension() == 0) {
        createDefaultEmbeddings();
        // Validate that defaults are non-zero; if zero, warn
        auto test = getTagEmbedding("warm");
        float norm = 0.0f; for (float v : test) norm += v*v; norm = std::sqrt(norm);
        if (norm < 1e-6f) {
            std::cerr << "Warning: default embeddings appear zero; encoder may be misconfigured" << std::endl;
        }
        // Update dimension after seeding
        int seededDim = db_->getEmbeddingDimension();
        if (seededDim > 0) dimension_ = seededDim;
    }
    
    // Ready
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
    
    // Count document frequency per canonical tag
    std::unordered_map<std::string, int> docFreq;
    for (const auto& doc : docs) {
        std::unordered_map<std::string, bool> seen;
        for (const auto& tag : doc) {
            std::string canonical = getCanonicalTag(tag);
            if (!seen[canonical]) {
                docFreq[canonical]++;
                seen[canonical] = true;
            }
        }
    }
    
    const int totalDocs = static_cast<int>(docs.size());
    int storedCount = 0;
    for (const auto& [tag, df] : docFreq) {
        // Avoid division by zero; also guard df>0
        if (df <= 0) continue;
        float idf = std::log(static_cast<float>(totalDocs) / static_cast<float>(df));
        if (db_->storeIDF(tag, idf, df)) {
            ++storedCount;
        }
    }
    std::cout << "Computed IDF for " << storedCount << " tags (" << totalDocs << " documents)" << std::endl;
    return storedCount;
}

bool SemanticKnowledgeBase::persistTrackerState(const nlohmann::json& state) {
    if (!db_) return false;
    return db_->storeTrackerState(state);
}

nlohmann::json SemanticKnowledgeBase::loadTrackerState() {
    if (!db_) return nlohmann::json();
    return db_->loadTrackerState();
}

void SemanticKnowledgeBase::createDefaultEmbeddings() {
    if (!db_) return;
    
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
    for (const auto& tagData : commonTags) {
        // Create a simple embedding based on semantic neighbors
        // This is a placeholder - real embeddings would be from a model
        std::vector<float> embedding(dimension_, 0.0f);
        
        // Generate hash-based embedding using sentence encoder
        if (encoder_) {
            embedding = encoder_->encode(tagData.tag);
        }
        
        if (db_->storeEmbedding(tagData.tag, embedding, tagData.canonical)) {
            ++storedCount;
        }
    }
    
    std::cout << "Created " << storedCount << " default embeddings" << std::endl;
}

} // namespace audio_config
