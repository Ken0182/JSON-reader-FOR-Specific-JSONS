/**
 * @file sentence_encoder.hpp
 * @brief Sentence Encoder Interface - Text to Vector Embedding
 * @author AI Assistant
 * @version 1.6
 * 
 * v1.6 Sentence Encoder:
 * - Interface for encoding arbitrary text into semantic vectors
 * - Supports multiple backends (ONNX, hash-based fallback)
 * - Enables unlimited vocabulary (not restricted to curated tags)
 * - Handles user queries with free-form language
 * 
 * ARCHITECTURE:
 * 
 * SentenceEncoder (abstract interface)
 *   ├─ ONNXEncoder (optional, if ONNX Runtime available)
 *   │   └─ Uses MiniLM/E5-small models for real semantic encoding
 *   └─ HashEncoder (fallback)
 *       └─ Token-based hashing for basic semantic approximation
 * 
 * USAGE:
 *   auto encoder = SentenceEncoder::createDefault(db, dimension);
 *   auto vec = encoder->encode("dreamy but not lush");
 *   // Returns normalized vector based on semantic meaning
 * 
 * DESIGN RATIONALE:
 * - Abstract interface allows swapping backends without code changes
 * - Hash fallback ensures system works without external models
 * - Database integration provides curated high-quality embeddings
 * - Open-ended: users can describe timbre/feel in any way
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "semantic_db.hpp"

namespace audio_config {

/**
 * @brief Abstract sentence encoder interface
 * 
 * Encodes arbitrary text into semantic embedding vectors.
 * Implementations can use neural models (ONNX), hash-based methods,
 * or hybrid approaches combining database lookups with fallback encoding.
 */
class SentenceEncoder {
public:
    virtual ~SentenceEncoder() = default;
    
    /**
     * @brief Encode text into semantic vector
     * @param text Input text (query, tag, description)
     * @return Normalized embedding vector
     */
    virtual std::vector<float> encode(const std::string& text) const = 0;
    
    /**
     * @brief Get embedding dimension
     * @return Vector dimension
     */
    virtual int getDimension() const = 0;
    
    /**
     * @brief Check if encoder is ready
     * @return true if encoder can encode text
     */
    virtual bool isReady() const = 0;
    
    /**
     * @brief Factory method: create default encoder
     * @param db Semantic database (for tag lookup)
     * @param dimension Target embedding dimension
     * @return Sentence encoder instance
     * 
     * Creates ONNXEncoder if available, otherwise HashEncoder
     */
    static std::unique_ptr<SentenceEncoder> createDefault(
        const SemanticDatabase* db, 
        int dimension = 100);
    
    /**
     * @brief Factory method: create hash-based encoder
     * @param db Semantic database (for tag lookup)
     * @param dimension Target embedding dimension
     * @return Hash-based encoder instance
     */
    static std::unique_ptr<SentenceEncoder> createHashEncoder(
        const SemanticDatabase* db,
        int dimension = 100);
};

/**
 * @brief Hash-based sentence encoder (fallback)
 * 
 * Uses token-based hashing with database lookup:
 * 1. Tokenize text (using TextUtils)
 * 2. Look up each token in database
 * 3. If found, use database embedding
 * 4. If not found, generate hash-based embedding
 * 5. Average all token embeddings
 * 6. L2-normalize result
 * 
 * CHARACTERISTICS:
 * - No external dependencies (pure C++)
 * - Fast (microseconds per query)
 * - Stable (same input → same output)
 * - Limited semantic understanding (but better than nothing)
 * - Improved by database curated embeddings
 */
class HashEncoder : public SentenceEncoder {
public:
    /**
     * @brief Constructor
     * @param db Semantic database for tag lookup (can be nullptr)
     * @param dimension Target embedding dimension
     */
    explicit HashEncoder(const SemanticDatabase* db, int dimension = 100);
    
    std::vector<float> encode(const std::string& text) const override;
    int getDimension() const override { return dimension_; }
    bool isReady() const override { return true; }
    
private:
    const SemanticDatabase* db_;
    int dimension_;
    
    /**
     * @brief Generate hash-based embedding for unknown token
     * @param token Token to embed
     * @return Embedding vector (not normalized)
     */
    std::vector<float> hashToken(const std::string& token) const;
    
    /**
     * @brief L2-normalize vector in place
     * @param vec Vector to normalize
     */
    void normalizeVector(std::vector<float>& vec) const;
};

/**
 * @brief ONNX-based sentence encoder (optional)
 * 
 * Uses pre-trained transformer models (MiniLM, E5-small) via ONNX Runtime.
 * Provides high-quality semantic embeddings with true language understanding.
 * 
 * NOTE: Only available if ONNX Runtime is linked.
 * Will fall back to HashEncoder if ONNX not available.
 * 
 * FUTURE IMPLEMENTATION:
 * - Load ONNX model from file
 * - Tokenize using model's tokenizer
 * - Run inference to get embeddings
 * - Apply pooling (mean/CLS)
 * - L2-normalize
 */
class ONNXEncoder : public SentenceEncoder {
public:
    explicit ONNXEncoder(const std::string& modelPath, int dimension = 384);
    
    std::vector<float> encode(const std::string& text) const override;
    int getDimension() const override { return dimension_; }
    bool isReady() const override { return false; }  // Not yet implemented
    
private:
    std::string modelPath_;
    int dimension_;
    
    // Future: ONNX Runtime session, tokenizer, etc.
};

} // namespace audio_config
