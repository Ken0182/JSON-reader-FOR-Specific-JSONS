/**
 * @file sentence_encoder.cpp
 * @brief Sentence Encoder Implementation
 * @author AI Assistant
 * @version 1.6
 */

#include "sentence_encoder.hpp"
#include "text_utils.hpp"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <cstdint>

namespace audio_config {

// ============================================================================
// Factory Methods
// ============================================================================

std::unique_ptr<SentenceEncoder> SentenceEncoder::createDefault(
    const SemanticDatabase* db, 
    int dimension) {
    
    // TODO: Check if ONNX Runtime is available
    // For now, always use HashEncoder
    return createHashEncoder(db, dimension);
}

std::unique_ptr<SentenceEncoder> SentenceEncoder::createHashEncoder(
    const SemanticDatabase* db,
    int dimension) {
    return std::make_unique<HashEncoder>(db, dimension);
}

// ============================================================================
// HashEncoder Implementation
// ============================================================================

HashEncoder::HashEncoder(const SemanticDatabase* db, int dimension)
    : db_(db), dimension_(dimension) {
}

std::vector<float> HashEncoder::encode(const std::string& text) const {
    if (text.empty()) {
        // Return unit vector for empty input (safe neutral prior)
        std::vector<float> neutral(dimension_, 1.0f / std::sqrt(static_cast<float>(dimension_)));
        return neutral;
    }
    
    // Tokenize text using shared utilities
    auto tokens = TextUtils::tokenize(text);
    if (tokens.empty()) {
        // Return unit vector for whitespace-only input (safe neutral prior)
        std::vector<float> neutral(dimension_, 1.0f / std::sqrt(static_cast<float>(dimension_)));
        return neutral;
    }
    
    // Accumulator for averaging
    std::vector<float> accumulated(dimension_, 0.0f);
    int foundCount = 0;
    
    // Process each token
    for (const auto& token : tokens) {
        std::vector<float> tokenVec;
        
        // Try database lookup first
        if (db_) {
            // Try direct lookup
            tokenVec = db_->getEmbedding(token);
            
            // If not found, try canonical form
            if (tokenVec.empty()) {
                std::string canonical = db_->getCanonicalTag(token);
                if (canonical != token) {
                    tokenVec = db_->getEmbedding(canonical);
                }
            }
        }
        
        // If still not found, generate hash-based embedding
        if (tokenVec.empty()) {
            tokenVec = hashToken(token);
        }
        
        // Ensure correct dimension
        if (static_cast<int>(tokenVec.size()) != dimension_) {
            tokenVec.resize(dimension_, 0.0f);
        }
        
        // Accumulate
        for (int i = 0; i < dimension_; ++i) {
            accumulated[i] += tokenVec[i];
        }
        ++foundCount;
    }
    
    // Average and normalize
    if (foundCount > 0) {
        float scale = 1.0f / foundCount;
        for (float& val : accumulated) {
            val *= scale;
        }
    }
    
    normalizeVector(accumulated);
    return accumulated;
}

std::vector<float> HashEncoder::hashToken(const std::string& token) const {
    std::vector<float> vec(dimension_, 0.0f);
    if (token.empty()) return vec;
    
    // Multi-hash approach for better distribution
    // Hash 1: Character-based with prime multipliers
    uint64_t hash1 = 0xcbf29ce484222325ULL;  // FNV-1a offset
    for (char c : token) {
        hash1 ^= static_cast<uint64_t>(c);
        hash1 *= 0x100000001b3ULL;  // FNV-1a prime
    }
    
    // Hash 2: N-gram based (pairs of characters)
    uint64_t hash2 = 0x9e3779b97f4a7c15ULL;  // Golden ratio
    for (size_t i = 0; i + 1 < token.size(); ++i) {
        uint64_t pair = (static_cast<uint64_t>(token[i]) << 8) | 
                        static_cast<uint64_t>(token[i + 1]);
        hash2 ^= pair;
        hash2 *= 0x517cc1b727220a95ULL;
    }
    
    // Generate vector components using multiple hash functions
    for (int i = 0; i < dimension_; ++i) {
        // Mix hash values with dimension index
        uint64_t h = hash1 + hash2 * (i + 1);
        h ^= (h >> 33);
        h *= 0xff51afd7ed558ccdULL;
        h ^= (h >> 33);
        h *= 0xc4ceb9fe1a85ec53ULL;
        h ^= (h >> 33);
        
        // Map to [-1, 1] using sign bit and fractional part
        float sign = (h & 1) ? 1.0f : -1.0f;
        float magnitude = static_cast<float>(h & 0xFFFFFF) / 0xFFFFFFf;
        vec[i] = sign * magnitude;
    }
    
    // Do NOT normalize here - let caller average and normalize
    return vec;
}

void HashEncoder::normalizeVector(std::vector<float>& vec) const {
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

// ============================================================================
// ONNXEncoder Implementation (Stub)
// ============================================================================

ONNXEncoder::ONNXEncoder(const std::string& modelPath, int dimension)
    : modelPath_(modelPath), dimension_(dimension) {
    // Future: Load ONNX model, initialize tokenizer
}

std::vector<float> ONNXEncoder::encode(const std::string& text) const {
    // Future: Run ONNX inference
    // For now, return zero vector
    (void)text;  // Suppress warning
    return std::vector<float>(dimension_, 0.0f);
}

} // namespace audio_config
